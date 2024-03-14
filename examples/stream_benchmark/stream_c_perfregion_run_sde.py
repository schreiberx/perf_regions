#! /usr/bin/env python2

# cputype (see './sde64 -help')
cputype='hsw'
#cputype='skx'
#cputype='skl'

##############################################

import os
import sys
import subprocess

cacheblock_size=64

if cputype == 'hsw':
    counters=['PAPI_L3_TCM']
    counters=['PAPI_L3_TCM', 'perf::LLC-STORE-MISSES', 'perf::LLC-LOAD-MISSES', 'ix86arch::LLC_MISSES']
    counters=['PAPI_L3_TCM', 'perf::PERF_COUNT_HW_CACHE_LL:READ', 'perf::PERF_COUNT_HW_CACHE_LL:WRITE', 'perf::PERF_COUNT_HW_CACHE_LL:MISS', 'perf::NODE-PREFETCH-MISSES']
    counters=['PAPI_L3_TCM', 'MEM_LOAD_UOPS_L3_MISS_RETIRED']
    #counters=['PAPI_L3_TCM', 'perf::LLC-LOAD-MISSES', 'perf::LLC-PREFETCH-MISSES']
    counters=[
        'PAPI_L3_TCM',
        'OFFCORE_RESPONSE_0:DMND_DATA_RD',
        'OFFCORE_REQUESTS:ALL_DATA_RD',
#        'MEM_LOAD_UOPS_L3_MISS_RETIRED:LOCAL_DRAM',
        'perf::LLC-STORE-MISSES',
        ]
    counters=[
#        'PAPI_L3_TCM',
#        'PAPI_DP_OPS',
#        'PAPI_VEC_DP',
#        'PAPI_SP_OPS',
#        'PAPI_VEC_SP',
        ]

elif cputype in ['skx', 'skl']:
    counters=[
        'PAPI_L3_TCM',
#        'OFFCORE_RESPONSE_0:L3_MISS_LOCAL',
#        'OFFCORE_RESPONSE_0:DMND_DATA_RD',
        'MEM_LOAD_UOPS_L3_MISS_RETIRED:LOCAL_DRAM',
        'PAPI_PRF_DM'
        ]
    counters=[
        'PAPI_L3_TCM',
#        'PAPI_DP_OPS',
#        'PAPI_VEC_DP',
#        'PAPI_SP_OPS',
#        'PAPI_VEC_SP',
        ]
#    counters=['PAPI_L3_TCM', 'PAPI_PRF_DM', 'perf::LLC-LOAD-MISSES']
#    counters=['PAPI_L3_TCM', 'PAPI_PRF_DM', 'perf::LLC-STORE-MISSES', 'perf::LLC-LOAD-MISSES']
    #counters=['PAPI_L3_TCM', 'PAPI_PRF_DM', 'perf::LLC-LOAD-MISSES', 'perf::LLC-PREFETCH-MISSES']

#perf::PERF_COUNT_HW_CACHE_LL:MISS


#
# Prepare environment
#
d = dict(os.environ)

if 'LD_LIBRARY_PATH' in d:
    d['LD_LIBRARY_PATH']='../../build:'+d['LD_LIBRARY_PATH']
else:
    d['LD_LIBRARY_PATH']='../../build'

d['LIST_COUNTERS']=','.join(counters)


#
# SDE executable
#
sde=d['HOME']+"/software/sde-external-7.58.0-2017-01-23-lin/sde64 -"+cputype+" --"


def execprog(prog, env):
    print("EXEC: "+prog)
    process=subprocess.Popen(prog.split(' '), env=d, stdout=subprocess.PIPE)
    return process.communicate()

out, err = execprog(sde+' papi_avail', d)
print(out)
print(err)

out, err = execprog(sde+' papi_native_avail', d)
print(out)
print(err)

out, err = execprog(sde+' ./stream_c_perfregions.exe', d)
print(out)
print(err)

print('*'*80)
print('* PERF REGION OUTPUT *')
print('*'*80)

lines = out.splitlines()



counters_template = {}
for c in counters:
    counters_template[c] = None

template = {
    'time' : None,
    'pc' : counters_template.copy()
}

tags = ['COPY', 'SCALE', 'ADD', 'TRIAD']

# setup data structure to store profiling data
prof_data={}
for t in tags:
    prof_data[t] = template.copy()

print(prof_data)


profiling_data = -1
for l in lines:
    if l == 'Performance counters profiling:':
        profiling_data = 0
    elif l == 'Timing profiling:':
        profiling_data = 1

    
    for t in tags:
        if l[0:len(t)] == t:
            perf_data = l.split('\t')
            if profiling_data == 0:
                # Performance counters
                for i in range(0, len(counters)):
                    prof_data[t]['pc'][counters[i]] = perf_data[i+1]
            elif profiling_data == 1:
                # Timing counters
                prof_data[t]['time'] = perf_data[1]
            else:
                print("FATAL ERROR")
                sys.exit(1)


print('*'*80)
print('* PERF REGION OUTPUT *')
print('*'*80)

sys.stdout.write('pc\t')
sys.stdout.write('time\t')
for c in counters:
    sys.stdout.write(c+'\t')
sys.stdout.write('bandwidth(MB/s)\t')
sys.stdout.write('\n')

for t in tags:
    p = prof_data[t]

    time = float(p['time'])

    total_cache_blocks = 0
    pc_l3_tcm = float(p['pc']['PAPI_L3_TCM'])
    total_cache_blocks += pc_l3_tcm

    if cputype in ['skx', 'skl']:
        pc_prf_dm = float(p['pc']['PAPI_PRF_DM'])
        total_cache_blocks += pc_prf_dm


    bandwidth = total_cache_blocks*cacheblock_size/(time*1024.0*1024.0)

    sys.stdout.write(t+'\t')
    sys.stdout.write(p['time']+'\t')
    for c in counters:
        sys.stdout.write(p['pc'][c]+'\t')
    sys.stdout.write(str(bandwidth)+'\t')
    sys.stdout.write('\n')

