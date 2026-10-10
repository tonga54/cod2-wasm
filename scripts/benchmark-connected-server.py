#!/usr/bin/env python3
"""Observe real connected players, UDP cadence/acks and CPU inside a QA container.

This does not create clients or claim an eight-player load from getinfo traffic.
Open/join the requested number of actual game clients before running it. Packet
payloads, names, addresses and credentials are never retained in the report.
"""
import argparse
import json
import shlex
import socket
import struct
import subprocess
import time
from pathlib import Path


def distribution(values):
    if not values:
        return None
    ordered = sorted(values)
    return {'samples': len(values), 'mean': sum(values) / len(values),
            'p95': ordered[int((len(ordered) - 1) * .95)],
            'p99': ordered[int((len(ordered) - 1) * .99)], 'max': ordered[-1]}


def cpu_stats():
    return dict((key, int(value)) for key, value in
                (line.split() for line in Path('/sys/fs/cgroup/cpu.stat').read_text().splitlines()))


def observe(args):
    # AF_PACKET requires the container's existing CAP_NET_RAW, as root. Never
    # expand container privileges or publish its private game UDP ports.
    # ETH_P_ALL receives outgoing frames as well as incoming IPv4 traffic.
    capture = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x0003))
    capture.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 2 * 1024 * 1024)
    # Kernel timestamps prevent Python/query scheduling from appearing as a
    # server packet gap when several already-arrived datagrams are drained.
    capture.setsockopt(socket.SOL_SOCKET, 35, 1)  # Linux SO_TIMESTAMPNS
    capture.settimeout(.1)
    query = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    query.connect(('127.0.0.1', args.port))
    query.settimeout(.4)
    before = cpu_stats()
    start = time.monotonic()
    deadline = start + args.seconds
    next_query = start
    peers = {}
    counts, rtts, pings, maps = [], [], [], set()
    lost_queries = 0
    while time.monotonic() < deadline:
        now = time.monotonic()
        if now >= next_query:
            next_query = now + 1
            sent = time.monotonic()
            query.send(b'\xff\xff\xff\xffgetstatus connected-benchmark')
            try:
                data = query.recv(65536)
                assert data.startswith(b'\xff\xff\xff\xffstatusResponse\n')
                rtts.append((time.monotonic() - sent) * 1000)
                lines = data[4:].decode('latin1').splitlines()
                fields = lines[1].split('\\')
                info = dict(zip(fields[1::2], fields[2::2]))
                maps.add(info['mapname'])
                current_pings = [int(line.split(' ', 2)[1]) for line in lines[2:] if line.strip()]
                counts.append(len(current_pings))
                pings.extend(current_pings)
            except (OSError, ValueError, AssertionError, KeyError):
                lost_queries += 1
        try:
            frame, ancillary, _flags, _address = capture.recvmsg(65536, 64)
        except socket.timeout:
            continue
        # Ethernet + IPv4 without fragmentation, then UDP. Ignore OOB queries
        # and voice/fragment packets when calculating ordered game sequences.
        if len(frame) < 42 or frame[12:14] != b'\x08\x00' or frame[23] != 17:
            continue
        ip_header = (frame[14] & 15) * 4
        if ip_header < 20 or struct.unpack_from('!H', frame, 20)[0] & 0x3fff:
            continue
        offset = 14 + ip_header
        if len(frame) < offset + 12:
            continue
        src, dst, length = struct.unpack_from('!HHH', frame, offset)
        if args.port not in (src, dst) or length < 12:
            continue
        payload = frame[offset + 8:offset + length]
        sequence = struct.unpack_from('<I', payload)[0]
        if sequence & 0x80000000:
            continue
        outgoing = src == args.port
        peer = dst if outgoing else src
        state = peers.setdefault(peer, {'out': 0, 'in': 0, 'gaps': [], 'acks': [],
                                      'sent': {}, 'lastTime': None, 'lastSeq': None,
                                      'lastAck': -1})
        timestamp = next(data for level, kind, data in ancillary
                         if level == socket.SOL_SOCKET and kind == 35)
        sec, nanosec = struct.unpack('ll', timestamp)
        now = sec + nanosec * 1e-9
        if outgoing:
            state['out'] += 1
            if state['lastSeq'] is not None and sequence > state['lastSeq']:
                state['gaps'].append((now - state['lastTime']) * 1000)
            state['lastSeq'], state['lastTime'] = sequence, now
            state['sent'][sequence] = now
            if len(state['sent']) > 256:
                del state['sent'][next(iter(state['sent']))]
        else:
            state['in'] += 1
            # Netchan sequence/qport + the original one-byte server ID precede
            # the plaintext message acknowledgement in normal client commands.
            if len(payload) >= 15:
                ack = struct.unpack_from('<i', payload, 7)[0]
                sent = state['sent'].get(ack)
                if ack > state['lastAck'] and sent is not None:
                    state['acks'].append((now - sent) * 1000)
                    state['lastAck'] = ack
    seconds = time.monotonic() - start
    after = cpu_stats()
    # Socket-level drops mean cadence observations are incomplete, not that
    # players lost those packets. Linux returns packet/drop counters here.
    packet_stats = struct.unpack('II', capture.getsockopt(263, 6, 8))
    capture.close()
    query.close()
    connected = [value for value in peers.values() if value['in'] and value['out']]
    report = {'seconds': seconds, 'expectedClients': args.expected_clients,
              'observedClients': len(connected), 'minStatusPlayers': min(counts, default=0),
              'maxStatusPlayers': max(counts, default=0), 'maps': sorted(maps),
              'cpuPercentOneCore': (after['usage_usec'] - before['usage_usec']) / seconds / 10000,
              'cpuThrottledMs': (after.get('throttled_usec', 0) - before.get('throttled_usec', 0)) / 1000,
              'localStatusRttMs': distribution(rtts), 'lostStatusQueries': lost_queries,
              'serverReportedPingMs': distribution(pings),
              'capturePackets': packet_stats[0], 'captureDrops': packet_stats[1],
              'clients': [{'outgoingPackets': item['out'], 'incomingPackets': item['in'],
                           'outgoingPacketsPerSecond': item['out'] / seconds,
                           'incomingPacketsPerSecond': item['in'] / seconds,
                           'serverPacketGapMs': distribution(item['gaps']),
                           'snapshotAckRttMs': distribution(item['acks'])} for item in connected],
              'limits': 'Server-interface cadence; ack RTT includes LAN, gateway and client scheduling. '
                        'Status RTT is container-local. This is not client FPS or a packet-loss measurement.'}
    report['passed'] = (len(connected) == args.expected_clients and
                        min(counts, default=0) == args.expected_clients and
                        max(counts, default=0) == args.expected_clients and
                        not lost_queries and not packet_stats[1])
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--container', default='cod2-pi-performance-qa-cod2-server-1')
    parser.add_argument('--seconds', type=int, default=60)
    parser.add_argument('--port', type=int, default=28960)
    parser.add_argument('--expected-clients', type=int, default=8)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--ssh')
    parser.add_argument('--ssh-control')
    parser.add_argument('--in-container', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    assert 5 <= args.seconds <= 3600 and 1 <= args.expected_clients <= 64
    assert 1 <= args.port <= 65535
    if args.in_container:
        report = observe(args)
    else:
        command = ['docker', 'exec', '-u', '0', '-i', args.container, 'python3', '-', '--in-container',
                   '--seconds', str(args.seconds), '--port', str(args.port),
                   '--expected-clients', str(args.expected_clients)]
        if args.ssh:
            command = ['ssh'] + (['-S', args.ssh_control] if args.ssh_control else []) + [args.ssh, shlex.join(command)]
        report = json.loads(subprocess.check_output(command, input=Path(__file__).read_bytes()))
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    if not report['passed'] and not args.in_container:
        raise SystemExit('Connected-player/capture checks failed; do not claim a complete load test.')


if __name__ == '__main__':
    main()
