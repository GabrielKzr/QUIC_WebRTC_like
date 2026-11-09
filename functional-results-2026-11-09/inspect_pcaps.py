from pathlib import Path
import struct, json
BASE=Path(__file__).resolve().parent
METHODS={1:'Binding Request',0x101:'Binding Success',0x11:'Binding Indication',3:'Allocate Request',0x103:'Allocate Success',0x113:'Allocate Error',4:'Refresh Request',0x104:'Refresh Success',0x114:'Refresh Error',8:'CreatePermission Request',0x108:'CreatePermission Success',9:'ChannelBind Request',0x109:'ChannelBind Success'}
def packets(path):
 data=path.read_bytes(); magic=data[:4]; endian='<' if magic in [bytes.fromhex('d4c3b2a1'),bytes.fromhex('4d3cb2a1')] else '>'
 nanos=magic in [bytes.fromhex('4d3cb2a1'),bytes.fromhex('a1b23c4d')]
 offset=24; idx=0; t0=None
 while offset<len(data):
  sec,frac,n,wire=struct.unpack_from(endian+'IIII',data,offset); offset+=16
  pkt=data[offset:offset+n]; offset+=n; idx+=1; t=sec+frac/(1e9 if nanos else 1e6)
  if t0 is None:t0=t
  if len(pkt)<34 or pkt[12:14]!=b'\x08\x00':continue
  ip=pkt[14:]; ihl=(ip[0]&15)*4; plen=struct.unpack_from('!H',ip,2)[0];ip=ip[:plen]
  src='.'.join(map(str,ip[12:16]));dst='.'.join(map(str,ip[16:20])); proto=ip[9];p=ip[ihl:]
  if len(p)<8:continue
  sp,dp=struct.unpack_from('!HH',p)
  rec={'frame':idx,'time':round(t-t0,6),'src':f'{src}:{sp}','dst':f'{dst}:{dp}'}
  if proto==17:
   body=p[8:];rec.update(proto='UDP',length=len(body))
   if len(body)>=20 and body[4:8]==bytes.fromhex('2112a442'):
    typ,ln=struct.unpack_from('!HH',body); rec['kind']=METHODS.get(typ,hex(typ));rec['txid']=body[8:20].hex(); attrs=[]; off=20
    while off+4<=min(len(body),20+ln):
     at,al=struct.unpack_from('!HH',body,off); val=body[off+4:off+4+al]; off+=4+((al+3)//4)*4
     if at==0x25:attrs.append('USE-CANDIDATE')
     elif at==0x08:attrs.append('MESSAGE-INTEGRITY')
     elif at==0x06:attrs.append('USERNAME')
     elif at==0x0c and len(val)>=2:rec['channel']=int.from_bytes(val[:2],'big')
     elif at==0x15:rec['nonce_hex']=val.hex()
     elif at==0x0d and len(val)==4:rec['lifetime']=int.from_bytes(val,'big')
     elif at==0x09 and len(val)>=4:rec['error']=val[2]*100+val[3]
     elif at in [0x20,0x16,0x12] and len(val)>=8 and val[1]==1:
      port=int.from_bytes(val[2:4],'big')^0x2112
      addr=bytes(a^b for a,b in zip(val[4:8],bytes.fromhex('2112a442')))
      rec[{0x20:'xor_mapped',0x16:'xor_relayed',0x12:'xor_peer'}[at]]='.'.join(map(str,addr))+':'+str(port)
    if attrs:rec['attributes']=attrs
   elif len(body)>=4 and (sp==3478 or dp==3478) and body[0]&0xc0==0x40 and int.from_bytes(body[2:4],'big')<=len(body)-4:
    rec['kind']='TURN ChannelData';rec['channel']=int.from_bytes(body[:2],'big'); rec['channel_length']=int.from_bytes(body[2:4],'big')
   elif len(body)>=6 and body[0]&0xc0==0xc0:
    rec['kind']='QUIC '+{0:'Initial',1:'0-RTT',2:'Handshake',3:'Retry'}[(body[0]>>4)&3]
   else:rec['kind']='UDP payload';rec['prefix']=body[:8].hex()
  elif proto==6 and len(p)>=20:
   h=(p[12]>>4)*4;body=p[h:];rec.update(proto='TCP',flags=p[13],length=len(body),seq=int.from_bytes(p[4:8],'big'),prefix=body[:16].hex(),data_hex=body.hex())
  else:continue
  yield rec
files={
 'remap_lan':'short/artifacts/network_ice_udp_transport_remap/pcap/peer_a/eth0.pcap',
 'remap_wan':'short/artifacts/network_ice_udp_transport_remap/pcap/router-a/wan.pcap',
 'quic_direct_wan':'short/artifacts/network_quic_ice_tcp/pcap/router-a/wan.pcap',
 'quic_direct_a_tcp':'short/artifacts/network_quic_ice_tcp/pcap/peer_a/lo.pcap',
 'quic_direct_b_tcp':'short/artifacts/network_quic_ice_tcp/pcap/peer_b/lo.pcap',
 'quic_turn_wan':'short/artifacts/network_quic_ice_turn_tcp/pcap/router-a/wan.pcap',
 'long_lan':'long/artifacts/network_quic_ice_turn_long/pcap/peer_a/eth0.pcap'}
out={key:{'path':str(Path(BASE.name)/path),'packets':list(packets(BASE/path))} for key,path in files.items()}
(BASE/'packet-index.json').write_text(json.dumps(out,indent=2)+'\n')
for key,item in out.items():
 print('\n'+key+' '+item['path'])
 ps=item['packets']
 if key.startswith('remap'):
  shown=[p for p in ps if p.get('kind') in ['Binding Request','Binding Success']]
 elif key=='quic_direct_wan':
  shown=[p for p in ps if 'kind' in p and (p['kind']!='UDP payload' or p['time']<2.5)]
 elif key.endswith('tcp'):
  shown=[p for p in ps if p['proto']=='TCP' and (p['flags']&3 or p['length']>0) and (':1234' in p['src'] or ':1234' in p['dst'] or ':44444' in p['src'] or ':44444' in p['dst'])]
 elif key=='quic_turn_wan':
  shown=[p for p in ps if p.get('kind') not in ['TURN ChannelData','Binding Indication','UDP payload']][:14]
  shown += [p for p in ps if p.get('kind')=='TURN ChannelData'][:3]
 else:
  shown=[p for p in ps if p.get('kind') in ['Refresh Request','Refresh Success','Refresh Error','CreatePermission Request','CreatePermission Success','ChannelBind Request','ChannelBind Success'] and p['time']>200]
 for p in shown[:50]:print(json.dumps(p))
