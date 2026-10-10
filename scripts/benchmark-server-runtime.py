#!/usr/bin/env python3
"""Sample an isolated QA container's CPU and direct getinfo RTT, without game bots."""
import argparse,json,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--container',default='cod2-gameplay-qa-cod2-server-1');p.add_argument('--output',required=True);args=p.parse_args()
probe=r'''
import json,socket,time
from pathlib import Path
started=time.monotonic();before=dict(x.split() for x in Path('/sys/fs/cgroup/cpu.stat').read_text().splitlines());rtts=[];lost=0
with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as udp:
 udp.settimeout(.5);udp.connect(('127.0.0.1',28960))
 for i in range(300):
  deadline=started+(i+1)*.05;t=time.monotonic();udp.send(b'\xff\xff\xff\xffgetinfo performance-probe')
  try:
   data=udp.recv(4096)
   if not data.startswith(b'\xff\xff\xff\xffinfoResponse'):raise OSError('bad response')
   rtts.append((time.monotonic()-t)*1000)
  except OSError:lost+=1
  delay=deadline-time.monotonic()
  if delay>0:time.sleep(delay)
end=time.monotonic();after=dict(x.split() for x in Path('/sys/fs/cgroup/cpu.stat').read_text().splitlines());rtts.sort()
print(json.dumps({'seconds':end-started,'cpuPercentOneCore':(int(after['usage_usec'])-int(before['usage_usec']))/(end-started)/10000,'throttledMs':(int(after.get('throttled_usec',0))-int(before.get('throttled_usec',0)))/1000,'infoRttMs':{'mean':sum(rtts)/len(rtts),'p95':rtts[int((len(rtts)-1)*.95)],'p99':rtts[int((len(rtts)-1)*.99)],'max':max(rtts)},'queries':300,'lost':lost}))
'''
result=subprocess.check_output(['docker','exec','-i',args.container,'python3','-c',probe]);report=json.loads(result);Path(args.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
