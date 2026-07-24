# How to use

Support command:

|  command                 | kernel 5.4  | kernel 5.15  |  kernel 6.1  | kernel 6.6   |       comment            |
|  --------                | --------    | --------     |  --------    |   --------   | -----------------        |
| [dts](#dts)              | √           | √            | √            | √            |  show device tree        |
| [cma](#cma)              | √           | √            | √            | √            |  show cma memory         |
| [reserved](#reserved)    | √           | √            | √            | √            |  show reserved memory    |
| [vmalloc](#vmalloc)      | √           | √            | √            | √            |  show vmalloc memory     |
| [memblock](#memblock)    | √           | √            | √            | √            |  show memblock memory    |
| [dmabuf](#dmabuf)        | √           | √            | √            | √            |  show dmabuf memory      |
| [buddy](#buddy)          | √           | √            | √            | √            |  show buddy info         |
| [zram](#zram)            | √           | √            | √            | √            |  show zram memory        |
| [meminfo](#meminfo)      | √           | √            | √            | √            |  show process memory     |
| [procrank](#procrank)    | √           | √            | √            | √            |  show process memory     |
| [binder](#binder)        | √           | √            | √            | √            |  show binder info        |
| [slub](#slub)            | √           | √            | √            | √            |  show slub memory        |
| [dd](#dd)                | √           | √            | √            | √            |  show device driver info |
| [wq](#wq)                | √           | √            | √            | √            |  show workqueue          |
| [df](#df)                | √           | √            | √            | √            |  show mount info         |
| [pageowner](#pageowner)  | √           | √            | √            | √            |  show pageowner          |
| [swap](#swap)            | √           | √            | √            | √            |  show swap info          |
| [rtb](#rtb)              | √           | √            | √            | √            |  show rtb log            |
| [cpu](#cpu)              | √           | √            | √            | √            |  show cpu frequency      |
| [coredump](#coredump)    | √           | √            | √            | √            |  generate coredump       |
| [tm](#tm)                | √           | √            | √            | √            |  show thermal info       |
| [wdt](#wdt)              | √           | √            | √            | √            |  show watchdog info      |
| [cache](#cache)          | √           | √            | √            | √            |  show pagecache info     |
| [dbi](#dbi)              | √           | √            | √            | √            |  show debug image info   |
| [ipc](#ipc)              | √           | √            | √            | √            |  show ipc log            |
| [reg](#reg)              | √           | √            | √            | √            |  show regulator info     |
| [icc](#icc)              | √           | √            | √            | √            |  show icc info           |
| [ccf](#ccf)              | √           | √            | √            | √            |  show clock info         |
| [pstore](#pstore)        | √           | √            | √            | √            |  show pstore log         |
| [env](#env)              | √           | √            | √            | √            |  show system info        |
| [qlog](#qlog)            | √           | √            | √            | √            |  show boot log info      |
| [sched](#sched)          | √           | √            | √            | √            |  show task sched info    |
| [systemd](#systemd)      | √           | √            | √            | √            |  show journal log        |
| [t32](#t32)              | √           | √            | √            | √            |  get t32.bat script      |
| [ftrace](#ftrace)        | ×           | √            | ×            | ×            |  parser trace event info |
| [lockdep](#lockdep)      | ×           | √            | ×            | ×            |  Lockdep detection       |
| [iommu](#iommu)          | √           | √            | √            | √            |  print iommu information |

|  command                 |   Android-11(30)  |  Android-12(31) |  Android-13(33) |     comment           |
|  --------                | ----------------  | --------------- | --------------- | -----------------     |
| [prop](#prop)            | √                 | √               | √               | show property         |
| [logcat](#logcat)        | √                 | √               | √               | show logcat log       |
| [sfg](#sfg)              | √                 | √               | √               | show layer info       |

## memblock
This command is used to view detailed information about memblock.

### memblock -a
```
crash> memblock -a
┌──────────────────────────────────────────────────────────────────────────────┐
│  Type:                memory                                                 │
│  memblock_type:       0xffffffdd7acdc4f8                                     │
│  Total Size:          5.75GB                                                 │
│  Region Count:        14                                                     │
├─────┬────────────────────┬────────────────────────────┬────────────┬─────────┤
│    #│memblock_region     │Physical Range              │Size        │Flags    │
├─────┼────────────────────┼────────────────────────────┼────────────┼─────────┤
│    0│0xffffffdd7b2b3e58  │[0x080e00000~0x081800000]   │        10MB│NONE     │
│    1│0xffffffdd7b2b3e70  │[0x081cf5000~0x081cff000]   │        40KB│NONE     │
│    2│0xffffffdd7b2b3e88  │[0x081f20000~0x0824a0000]   │      5.50MB│NONE     │
│    3│0xffffffdd7b2b3ea0  │[0x082800000~0x099280000]   │    362.50MB│NONE     │
│    8│0xffffffdd7b2b3f18  │[0x0ea700000~0x0fc800000]   │       289MB│NONE     │
│    9│0xffffffdd7b2b3f30  │[0x0fca00000~0x100000000]   │        54MB│NONE     │
│   10│0xffffffdd7b2b3f48  │[0x880000000~0x8afbff000]   │    764.00MB│NONE     │
│   11│0xffffffdd7b2b3f60  │[0x8b0000000~0x8ba700000]   │       167MB│NONE     │
│   12│0xffffffdd7b2b3f78  │[0x8bf800000~0x97ffff000]   │      3.01GB│NONE     │
│   13│0xffffffdd7b2b3f90  │[0x97ffff000~0x980000000]   │         4KB│NO-MAP   │
└─────┴────────────────────┴────────────────────────────┴────────────┴─────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│  Type:                reserved                                               │
│  memblock_type:       0xffffffdd7acdc520                                     │
│  Total Size:          429.90MB                                               │
│  Region Count:        55                                                     │
├─────┬────────────────────┬────────────────────────────┬────────────┬─────────┤
│    #│memblock_region     │Physical Range              │Size        │Flags    │
├─────┼────────────────────┼────────────────────────────┼────────────┼─────────┤
│    0│0xffffffdd7b2b9e58  │[0x082800000~0x084800000]   │        32MB│NONE     │
│    1│0xffffffdd7b2b9e70  │[0x0a8010000~0x0ab257000]   │     50.28MB│NONE     │
│    2│0xffffffdd7b2b9e88  │[0x0ab25a000~0x0ab260000]   │        24KB│NONE     │
│    3│0xffffffdd7b2b9ea0  │[0x0affffb80~0x0b0000000]   │      1.12KB│NONE     │
│   53│0xffffffdd7b2ba350  │[0x97d8c5a28~0x97d8c5a54]   │         44B│NONE     │
│   54│0xffffffdd7b2ba368  │[0x97d8c5a58~0x980000000]   │     39.23MB│NONE     │
└─────┴────────────────────┴────────────────────────────┴────────────┴─────────┘

Summary:
┌──────────────────────────────────────────────────────────────────────────────┐
│  Total Memory Regions:    69                                                 │
│  Memory Size:             5.75GB                                             │
│  Reserved Size:           429.90MB                                           │
│  Bottom Up:               No                                                 │
└──────────────────────────────────────────────────────────────────────────────┘
```
## reserved
This command is used to view detailed information about reserved memory.
### reserved -a
```
crash> reserved -a
Reserved Memory Regions Overview
┌────┬──────────────────────────────────┬──────────────────┬───────────────────────────┬──────────┬──────────┐
│   #│Region Name                       │Base Address      │Physical Memory Range      │Total Size│Status    │
├────┼──────────────────────────────────┼──────────────────┼───────────────────────────┼──────────┼──────────┤
│ 33 │splash_region                     │0x00000000e3940000│[0x0e3940000~0x0e6440000]  │      43MB│Unknow    │
│ 34 │qtee_region@e8900000              │0x00000000e8900000│[0x0e8900000~0x0e8e00000]  │       5MB│NO-MAP    │
│ 35 │tags_region@e8e00000              │0x00000000e8e00000│[0x0e8e00000~0x0e9500000]  │       7MB│NO-MAP    │
│ 36 │trusted_apps_region@e9500000      │0x00000000e9500000│[0x0e9500000~0x0ea700000]  │      18MB│NO-MAP    │
│ 37 │linux,cma                         │0x00000000f6800000│[0x0f6800000~0x0f8800000]  │      32MB│REUSABLE  │
│ 44 │mem_dump_region                   │0x000000097e400000│[0x97e400000~0x97fc00000]  │      24MB│REUSABLE  │
├────┴──────────────────────────────────┴──────────────────┴───────────────────────────┴──────────┴──────────┤
│ Regions: 45 | Total: 403.57MB | Nomap: 168.57MB | Reuse: 192MB | Unknow: 43MB                              │
└────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```
## cma
This command is used to view detailed information about cma memory.
### cma -a
View cma memory information
```
crash> cma -a
CMA (Contiguous Memory Allocator) Areas Overview
┌────┬───────────────────────────┬──────────────────┬───────────────────────────┬──────────┬──────────┬────────┬──────┐
│  # │CMA Area Name              │CMA Address       │Physical Memory Range      │Total Size│ Used Size│  Usage%│ Order│
├────┼───────────────────────────┼──────────────────┼───────────────────────────┼──────────┼──────────┼────────┼──────┤
│  1 │qdss_apps_region@82800000  │0xffffffdd7b88e298│[0x082800000~0x084800000]  │      32MB│        0B│    0.0%│     0│
│  2 │linux,cma                  │0xffffffdd7b88e188│[0x0f6800000~0x0f8800000]  │      32MB│   17.66MB│   55.2%│     0│
│  3 │qseecom_ta_region          │0xffffffdd7b88e078│[0x0f8800000~0x0f9800000]  │      16MB│        0B│    0.0%│     0│
│  4 │adsp_heap_region           │0xffffffdd7b88df68│[0x0f9800000~0x0fa400000]  │      12MB│    5.20MB│   43.3%│     0│
│  5 │qseecom_region             │0xffffffdd7b88de58│[0x0fa400000~0x0fb800000]  │      20MB│    1.06MB│    5.3%│     0│
│  6 │va_md_mem_region           │0xffffffdd7b88dc38│[0x0fb800000~0x0fc800000]  │      16MB│        0B│    0.0%│     0│
│  7 │secure_cdsp_region         │0xffffffdd7b88db28│[0x0fd400000~0x0ffc00000]  │      40MB│        0B│    0.0%│     0│
│  8 │mem_dump_region            │0xffffffdd7b88dd48│[0x97e400000~0x97fc00000]  │      24MB│   20.66MB│   86.1%│     0│
├────┴───────────────────────────┴──────────────────┴───────────────────────────┴──────────┴──────────┴────────┴──────┤
│ Areas: 8 | Total: 192MB | Used: 44.57MB (23.2%)                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```
### cma -u 'cma name'
Check page status by physical addr.
```
crash> cma -p 0x082810000
┌──────────────────────────────────────────────────────────────────────────────┐
│  CMA Region:              qdss_apps_region@82800000                          │
│  Total Size:              32MB                                               │
│  Address Range:           [0x082800000~0x084800000]                          │
└──────────────────────────────────────────────────────────────────────────────┘
Page Information:
┌──────────────────────────────────────────────────────────────────────────────┐
│  Bit Index:               16                                                 │
│  Page Range:              [0x082810000~0x082811000]                          │
│  Page Size:               4KB                                                │
│  Status:                  FREE                                               │
└──────────────────────────────────────────────────────────────────────────────┘
```

## buddy
This command is used to view detailed information about buddy memory.
### buddy -a
View all buddy memory information.
```
crash> buddy -a

Node(0)
---------------------------------------------------------------------------------------------------------------------------
                                             zone Normal
---------------------------------------------------------------------------------------------------------------------------
       Order      4KB      8KB     16KB     32KB     64KB    128KB    256KB    512KB      1MB      2MB      4MB      Total
   Unmovable       45      545      261      173       57        0        0        0        0        0        0    17.48MB
     Movable     2855      998       12        0        3        0        1        0        0        0        0    19.57MB
 Reclaimable        7        0       43        2        1        0        0        0        0        0        0      844KB
         CMA        0        0        0        0        0        0        0        0        0        0        0         0B
  HighAtomic        0        0        1        2        1        3        0        2        1        0        0     2.52MB
     Isolate        0        0        0        0        0        0        0        0        0        0        0         0B
       Total  11.36MB  12.05MB   4.95MB   5.53MB   3.88MB    384KB    256KB      1MB      1MB       0B       0B    40.39MB
---------------------------------------------------------------------------------------------------------------------------
```
### buddy -n
Display memory node info.
```
crash> buddy -n

Config:
---------------------------------------
min_free_kbytes          : 3350kB
user_min_free_kbytes     : 3350kB
watermark_scale_factor   : 1
---------------------------------------

Node:
=======================================
pglist_data(0): c2081180
  spanned           : 259712(1014.50MB)
  present           : 221272(864.34MB)
  hole              : 38440(150.16MB)
  start_pfn         : 40000
  start_paddr       : 40000000

  Normal zone:c2081180
    spanned         : 230656(901MB)
    present         : 192216(750.84MB)
    hole            : 38440(150.16MB)
    managed         : 180077(703.43MB)
    reserved        : 12139(47.42MB)
    start_pfn       : 40000
    start_paddr     : 40000000
    watermark_boost : 0
    WMARK_HIGH      : 1651(6.45MB)
    WMARK_LOW       : 1442(5.63MB)
    WMARK_MIN       : 837(3.27MB)
```
### buddy -z 'zone addr'
Display page info of zone.
crash> buddy -z c2081180
 ```
Order[0] 4KB
   migratetype:Unmovable Order[0]
     [00001]Page:f7cfc080 PA:46b20000
     [00002]Page:f80a6f78 PA:60c6e000
     [00003]Page:f7c76508 PA:42fb2000
     [00004]Page:f7fc7298 PA:5a8f6000
   migratetype:Movable Order[0]
     [00001]Page:f8066f94 PA:5effd000
     [00002]Page:f7d2068c PA:47b4b000
     [00003]Page:f8153d90 PA:65944000
     [00004]Page:f80e93c8 PA:629e2000
     [00005]Page:f803becc PA:5dcdb000
```
## procrank
This command dumps the process info. you must insmod the zram ko before run this command.

### procrank -a
```
crash> mod -s zram ./lib/zram.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d3758440  zram                       ffffffd4d3752000     49152  ./lib/zram.ko
crash> mod -s zsmalloc ./lib/zsmalloc.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d34c6200  zsmalloc                   ffffffd4d34bf000     57344  ./lib/zsmalloc.ko

crash> procrank -a
PID      Vss        Rss        Pss        Uss        Swap       Comm
2023     1.25GB     85.30MB    24.93MB    11.47MB    34.68MB    com.android.systemui
1067     2.41GB     84.20MB    20.58MB    6.70MB     47.02MB    system_server
4067     1.01GB     64.66MB    10.31MB    1.38MB     25.33MB    com.qualcomm.qti.weartech.watchface
4268     1.06GB     62.55MB    12.78MB    3.62MB     22.26MB    com.android.settings
4088     1.00GB     58.61MB    8.34MB     1.11MB     25.54MB    com.android.inputmethod.latin
3866     1.50GB     42.37MB    5.44MB     328KB      23.07MB    com.android.commands.monkey
2807     1.03GB     42.34MB    3.34MB     240KB      26.62MB    com.android.providers.media.module
712      1.66GB     39.58MB    2.66MB     40KB       24.52MB    zygote
2278     1017.48MB  36.51MB    2.11MB     0B         25.37MB    com.qualcomm.qti.standby
2348     1.04GB     36.39MB    2.08MB     8KB        32.59MB    com.android.phone
2828     1.00GB     36.23MB    1.90MB     4KB        25.74MB    com.android.nfc
2143     1022.46MB  35.62MB    1.87MB     4KB        26.03MB    com.android.networkstack.process
2281     1.00GB     34.75MB    1.72MB     4KB        25.66MB    .qtidataservices
2851     1020.70MB  34.48MB    1.75MB     8KB        28.47MB    com.qualcomm.location
2865     1021.14MB  34.44MB    1.68MB     0B         25.35MB    com.qti.diagservices
2292     1017.32MB  33.75MB    1.64MB     0B         25.34MB    com.qualcomm.qti.aonutility
```

### procrank -c
```
crash> mod -s zram ./lib/zram.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d3758440  zram                       ffffffd4d3752000     49152  ./lib/zram.ko
crash> mod -s zsmalloc ./lib/zsmalloc.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d34c6200  zsmalloc                   ffffffd4d34bf000     57344  ./lib/zsmalloc.ko

crash> procrank -c
PID      Comm                 Cmdline
1        init                 /system/bin/init
```

## vmalloc
This command dumps the vmalloc info.

### vmalloc -a
Display vmalloc memory info
```
crash> vmalloc -a
[0000]vmap_area:ffffff8003015540 range:[ffffffc008000000~ffffffc008005000] size:20KB
   vm_struct:ffffff8003003a00 size:20KB flags:vmalloc nr_pages:4 addr:ffffffc008000000 phys_addr:0 start_kernel+496
       [0001]Page:fffffffe000c2a00 PA:430a8000
       [0002]Page:fffffffe000c2a40 PA:430a9000
       [0003]Page:fffffffe000c2a80 PA:430aa000
       [0004]Page:fffffffe000c2ac0 PA:430ab000

[0001]vmap_area:ffffff8003015a00 range:[ffffffc008005000~ffffffc008007000] size:8KB
   vm_struct:ffffff8003003c40 size:8KB flags:vmalloc nr_pages:1 addr:ffffffc008005000 phys_addr:0 init_IRQ+344
       [0001]Page:fffffffe000c2ec0 PA:430bb000
```
### vmalloc -r
Display all vmap_area info
```
crash> vmalloc -r
Total vm size:416.71MB
==============================================================================================================
[0000]vmap_area:ffffff8003015540 range:[ffffffc008000000~ffffffc008005000] size:20KB
[0001]vmap_area:ffffff8003015a00 range:[ffffffc008005000~ffffffc008007000] size:8KB
[0002]vmap_area:ffffff8003015940 range:[ffffffc008008000~ffffffc00800d000] size:20KB
[0003]vmap_area:ffffff8003015500 range:[ffffffc00800d000~ffffffc00800f000] size:8KB
[0004]vmap_area:ffffff8003015880 range:[ffffffc008010000~ffffffc008015000] size:20KB
[0005]vmap_area:ffffff8003015980 range:[ffffffc008015000~ffffffc008017000] size:8KB
```
### vmalloc -v
Display all vm_struct info
```
crash> vmalloc -v
Total vm size:475.16MB, physical size:101.41MB
==============================================================================================================
[0000]vm_struct:ffffff8003003a00 size:20KB     flags:vmalloc  nr_pages:4    kaddr:ffffffc008000000 phys_addr:0
[0001]vm_struct:ffffff8003003c40 size:8KB      flags:vmalloc  nr_pages:1    kaddr:ffffffc008005000 phys_addr:0
[0002]vm_struct:ffffff8003003ac0 size:20KB     flags:vmalloc  nr_pages:4    kaddr:ffffffc008008000 phys_addr:0
[0003]vm_struct:ffffff8003003cc0 size:8KB      flags:vmalloc  nr_pages:1    kaddr:ffffffc00800d000 phys_addr:0
```

### vmalloc -s
Display vmalloc statistical info
```
crash> vmalloc -s
Summary by caller:
========================================================
Func Name                                    virt            phys
map_lowmem                                   1.24GB          0B
__devm_ioremap+92                            56.38MB         0B
pmd_empty_section_gap                        35MB            0B
tmc_alloc_sg_table+184                       32.00MB         0B
pci_reserve_io                               24MB            0B
av8l_fast_alloc_pgtable+964                  12.02MB         0B
load_module+3876                             9.41MB          9.30MB

Summary by type:
========================================================
Type      virt            phys
unknow    1.24GB          0B
ioremap   143.93MB        0B
vmap      35.86MB         0B
vmalloc   21.27MB         18.20MB
vpages    10.12MB         0B

```
### vmalloc -f 'func name'
Display the allocated pages by function name
```
crash> vmalloc -f load_module
[0001]Page:fffffffe0054f780 PA:553de000
[0002]Page:fffffffe0054f740 PA:553dd000
[0003]Page:fffffffe00588a80 PA:5622a000
[0004]Page:fffffffe00588ac0 PA:5622b000
[0005]Page:fffffffe0065da80 PA:5976a000
[0006]Page:fffffffe007a3c40 PA:5e8f1000
```

### vmalloc -t 'type name'
Display vmalloc statistical info
```
crash> vmalloc -t vmalloc
[0001]Page:fffffffe000c2a00 PA:430a8000
[0002]Page:fffffffe000c2a40 PA:430a9000
[0003]Page:fffffffe000c2a80 PA:430aa000
[0004]Page:fffffffe000c2ac0 PA:430ab000
[0005]Page:fffffffe000c2ec0 PA:430bb000
```

## dmabuf
This command dumps the dmabuf information.

### dmabuf -b
Display all dmabuf.
```
crash> dmabuf -b
=======================================================================================
[001]dma_buf:c95483cc ref:2  priv:c31f9a80 ops::dma_buf_ops  [ion_dma_buf] size:648KB
[002]dma_buf:c87163cc ref:6  priv:c9d26e80 ops::dma_buf_ops  [ion_dma_buf] size:648KB
[003]dma_buf:db095c4c ref:6  priv:d72dbe80 ops::dma_buf_ops  [ion_dma_buf] size:648KB
=======================================================================================
Total size:12.89MB
```

### dmabuf -B 'dma_buf address'
Display the dmabuf detail info by dma_buf address.
```
crash> dmabuf -B c95483cc
dma_buf:c95483cc ref:2  priv:c31f9a80  [ion_dma_buf] sg_table:d6d4bd00 size:648KB
        pid:604   [allocator-servi] fd:7
        pid:632   [surfaceflinger] fd:35
        scatterlist:c3d94000 page:ec606740 offset:0 length:4KB dma_address:55350000 dma_length:0B
        scatterlist:c3d94014 page:ec47924c offset:0 length:4KB dma_address:4a2bb000 dma_length:0B
        scatterlist:c3d94028 page:ec3cf0a4 offset:0 length:4KB dma_address:45721000 dma_length:0B

```

### dmabuf -h
Display all heap info.
```
Id  ion_heap   type                   Name          flags  ops                      buf_cnt total_size
25  e8514008                          system        1      system_heap_ops          44      12.16MB
10  e9021a10                          secure_heap   0      system_secure_heap_ops   0       0B
6   e9a96808   ION_HEAP_TYPE_DMA      user_contig   0      ion_cma_ops              0       0B
7   e9a97408   ION_HEAP_TYPE_DMA      qsecom        0      ion_cma_ops              12      752KB
1   e9a96b08   ION_HEAP_TYPE_DMA      qsecom_ta     0      ion_cma_ops              0       0B

```

### dmabuf -H 'heap name'
Display the dmabuf detail info by heap name.
```
crash> dmabuf -H system
dma_buf:c95491cc ref:4  priv:c31f8680  [ion_dma_buf] sg_table:d3617080 size:32KB
        pid:604   [allocator-servi] fd:8
        pid:632   [surfaceflinger] fd:99
        scatterlist:c9548200 page:ec5d63ec offset:0 length:4KB dma_address:53de3000 dma_length:0B
        scatterlist:c9548214 page:ec5de54c offset:0 length:4KB dma_address:5417b000 dma_length:0B
        scatterlist:c9548228 page:ec8ef364 offset:0 length:4KB dma_address:69e51000 dma_length:0B
        scatterlist:c954823c page:ec639a40 offset:0 length:4KB dma_address:56a10000 dma_length:0B
        scatterlist:c9548250 page:ec3bc954 offset:0 length:4KB dma_address:44eed000 dma_length:0B
        scatterlist:c9548264 page:ec472de4 offset:0 length:4KB dma_address:49ff1000 dma_length:0B
        scatterlist:c9548278 page:ec3b2580 offset:0 length:4KB dma_address:44a60000 dma_length:0B
        scatterlist:c954828c page:ec472dc0 offset:0 length:4KB dma_address:49ff0000 dma_length:0B
```

### dmabuf -p
Display dmabuf size of process.
```
crash> dmabuf -p
PID   Comm                 buf_cnt  total_size
604   allocator-servi      2        680KB
605   composer-servic      22       7.30MB
632   surfaceflinger       34       10.66MB
```

### dmabuf -P 'pid'
Display the dmabuf detail info by pid.
```
crash> dmabuf -P 632
dma_buf:db0951cc ref:9  priv:d616bc80  [ion_dma_buf] sg_table:c9a871c0 size:32KB
        pid:605   [composer-servic] fd:33
        pid:632   [surfaceflinger] fd:100
        pid:1670  [arable.launcher] fd:135
        scatterlist:d6fc7380 page:ec41a854 offset:0 length:4KB dma_address:478ad000 dma_length:0B
        scatterlist:d6fc7394 page:ec4851b0 offset:0 length:4KB dma_address:4a80c000 dma_length:0B
        scatterlist:d6fc73a8 page:ec62b2a4 offset:0 length:4KB dma_address:563a1000 dma_length:0B
        scatterlist:d6fc73bc page:ec70835c offset:0 length:4KB dma_address:5c5df000 dma_length:0B
        scatterlist:d6fc73d0 page:ec5d932c offset:0 length:4KB dma_address:53f33000 dma_length:0B
        scatterlist:d6fc73e4 page:ec65690c offset:0 length:4KB dma_address:576eb000 dma_length:0B
        scatterlist:d6fc73f8 page:ec430c1c offset:0 length:4KB dma_address:4828f000 dma_length:0B
        scatterlist:d6fc740c page:ec447098 offset:0 length:4KB dma_address:48c76000 dma_length:0B
```

### dmabuf -s
Display the memory pool of system heap.
```
crash> dmabuf -s
system:
   page_pool          order high       low        total
   ffffff8024374a80   9     0B         108MB      108MB
   ffffff8024374240   4     0B         79.69MB    79.69MB
   ffffff80243743c0   0     0B         368KB      368KB
```

### dmabuf -S 'dma_buf address'
Save a dmabuf data to file.
```
crash> dmabuf -S ffffff881ced9400
Save dmabuf to file ./dma_buf@ffffff881ced9400.data !
```

## meminfo
This command dumps the memory information.

### meminfo -a
Display whole memory info.
```
crash> meminfo -a
MemTotal:         1849960 KB
MemFree:           491092 KB
MemAvailable:      548164 KB
Buffers:             1420 KB
Cached:            268632 KB
SwapCached:           580 KB
Active:            580572 KB
Inactive:          287124 KB
Active(anon):      437992 KB
Inactive(anon):    167244 KB
Active(file):      142580 KB
Inactive(file):    119880 KB
Unevictable:         2660 KB
Mlocked:             2656 KB
SwapTotal:              0 KB
SwapFree:               0 KB
Dirty:                196 KB
Writeback:              0 KB
AnonPages:         599768 KB
Mapped:            171416 KB
Shmem:               6296 KB
KReclaimable:      112088 KB
Slab:                1160 KB
SReclaimable:         580 KB
SUnreclaim:           580 KB
KernelStack:         2769 KB
PageTables:             0 KB
NFS_Unstable:           0 KB
Bounce:                 0 KB
WritebackTmp:           0 KB
CommitLimit:       924980 KB
VmallocTotal:     1835008 KB
VmallocUsed:        20404 KB
VmallocChunk:           0 KB
Percpu:              2784 KB
CmaTotal:          102400 KB
CmaFree:                0 KB
```
### meminfo -b
Breakdown memory info.
```
crash> meminfo -b
RAM                :     11.28GB
   MemTotal        :     10.98GB
   MemFree         :      5.30GB
   Buffers         :      9.59MB
   Cached          :      2.06GB
   SwapCached      :          0B
   AonPage         :      1.20GB
   FilePage        :      1.93GB
   Slab            :    962.71MB
   KernelStack     :     56.22MB
   PageTables      :    123.91MB
   Shmem           :     35.79MB
   Cma             :       604MB
   Dmabuf          :    143.18MB
   Vmalloc         :    171.66MB
   Other           :      1.08GB

   Struct Page     :    671.64MB
   Kernel Code     :     29.44MB
   Kernel Data     :     14.50MB
   Dentry Cache    :        16MB
   Inode  Cache    :        32KB

   NON_HLOS        :    481.45MB
```
### meminfo -v
Breakdown vmstat info.
```
crash> meminfo -v
Zone (DMA)                   Pages |       Size
present_pages:              524288 |   2048.0 MB
spanned_pages:              524288 |   2048.0 MB
managed_pages:              437462 |   1708.8 MB
cma_pages:                    8192 |     32.0 MB
... ...
WMARK_HIGH:                   3762 |     14.7 MB
WMARK_LOW:                    3135 |     12.2 MB
WMARK_MIN:                    2508 |      9.8 MB
WMARK_PROMO:                  4389 |     17.1 MB
NR_ZSPAGES:                      0 |      0.0 MB

Zone (Normal)                Pages |       Size
present_pages:             1554432 |   6072.0 MB
spanned_pages:             1554432 |   6072.0 MB
managed_pages:             1513233 |   5911.1 MB
cma_pages:                       0 |      0.0 MB
... ...
WMARK_HIGH:                  13131 |     51.3 MB
WMARK_LOW:                   10943 |     42.7 MB
WMARK_MIN:                    8755 |     34.2 MB
WMARK_PROMO:                 15319 |     59.8 MB
NR_ZSPAGES:                      3 |      0.0 MB

Global Stats                 Pages |       Size
NR_BOUNCE:                       0 |      0.0 MB
NR_FREE_CMA_PAGES:            7424 |     29.0 MB
NR_FREE_PAGES:             1776755 |   6940.4 MB
... ...
NR_ZSPAGES:                      3 |      0.0 MB

Node Stats                   Pages |       Size
NR_ACTIVE_ANON:                252 |      1.0 MB
NR_ACTIVE_FILE:              10066 |     39.3 MB
NR_ANON_MAPPED:              23535 |     91.9 MB
NR_ANON_THPS:                 9728 |     38.0 MB
... ...
NR_SLAB_RECLAIMABLE_B:       22097 |     86.3 MB
NR_SLAB_UNRECLAIMABLE_B:     29234 |    114.2 MB


VM EVENT Stats               Pages |       Size
PGACTIVATE:                  10054 |     39.3 MB
PGALLOC_NORMAL:             650263 |   2540.1 MB
PGFAULT:                    423976 |   1656.2 MB
PGFREE:                    2433164 |   9504.5 MB
... ...
PGMIGRATE_SUCCESS:               0 |      0.0 MB
PGPGIN:                     353158 |   1379.5 MB
UNEVICTABLE_PGCULLED:         9094 |     35.5 MB

```
## zram
This command dumps the zram info.

### zram -a
Display all zram info
```
crash> zram -a
========================================================================
zram                : ffffff801530dc00
name                : zram0
compressor          : lz4 rle
total_size          : 1.50GB
zs_pool             : ffffff805bad4000
orig_data_size      : 291.87MB
compr_data_size     : 88.53MB
compress ratio      : 30.33%
mem_used_max        : 93.48MB
mem_used_total      : 93.47MB
mem_limit           : 0B
same_pages          : 23.81MB
huge_pages          : 7.67MB
compacted_pages     : 17.56MB
========================================================================
```
### zram -m 'zram addr'
Display the memory pool of specified zram by zram address
```
crash> zram -m ffffff801530dc00
zs_pool             : ffffff805bad4000
name                : zram0
pages_allocated     : 23929
isolated_pages      : 0
pages_compacted     : 4495
=============================================================================================
index size_class       EMPTY ALMOST_EMPTY ALMOST_FULL  FULL  pages/zspage objs/zspage  obj_size  OBJ_ALLOCATED OBJ_USED
00000 ffffff805b0b7000 0     0            0            0     1            128          32B       0             0
00001 ffffff805b0b7300 0     1            2            7     3            256          48B       2560          2368
00002 ffffff805b0b7480 0     1            3            19    1            64           64B       1472          1444
00003 ffffff805b0b7f00 0     0            1            22    1            51           80B       1173          1168
00004 ffffff805b0b7cc0 0     0            1            6     3            128          96B       896           888
00005 ffffff805b0b7840 0     1            1            9     2            73           112B      803           782
00006 ffffff805b0b7a80 0     1            0            15    1            32           128B      512           501
00007 ffffff805b0b73c0 0     1            1            3     3            85           144B      425           382
```

### zram -f 'zram addr'
Display the full info of specified zram by zram address
```
crash> zram -f ffffff801530dc00
size_class(ffffff805b0b7300) objs_per_zspage:256 pages_per_zspage:3 size:48
   zspage[0]:ffffff805414f400 freeobj:66 inuse:66 class:1 fullness:1
       page[0]:fffffffe01e2b880 PFN:b8ae2 range:b8ae2000-b8ae3000 offset:0
           obj[000000]b8ae2000~b8ae2030 handle:ffffff8068ea93d0 index:0    alloc
           obj[000001]b8ae2030~b8ae2060 handle:ffffff8068d795e0 index:1    alloc
           obj[000002]b8ae2060~b8ae2090 handle:ffffff8013df55e0 index:2    alloc
           obj[000003]b8ae2090~b8ae20c0 handle:ffffff806b06ccd0 index:3    alloc
           obj[000004]b8ae20c0~b8ae20f0 handle:ffffff8068e612e0 index:4    alloc
           obj[000005]b8ae20f0~b8ae2120 handle:ffffff805b884520 index:5    alloc
```

### zram -p 'zram addr'
Display all pages of specified zram by zram address
```
crash> zram -p ffffff801530dc00
Page[00001]:fffffffe01e2b880 PFN:b8ae2 range:[b8ae2000-b8ae3000] offset:0
Page[00002]:fffffffe01f980c0 PFN:be603 range:[be603000-be604000] offset:32
Page[00003]:fffffffe01cf6440 PFN:b3d91 range:[b3d91000-b3d92000] offset:16
Page[00004]:fffffffe01e70c40 PFN:b9c31 range:[b9c31000-b9c32000] offset:0
Page[00005]:fffffffe01ef79c0 PFN:bbde7 range:[bbde7000-bbde8000] offset:32
Page[00006]:fffffffe01ee2000 PFN:bb880 range:[bb880000-bb881000] offset:16
Page[00007]:fffffffe01d6efc0 PFN:b5bbf range:[b5bbf000-b5bc0000] offset:0
Page[00008]:fffffffe01d54580 PFN:b5516 range:[b5516000-b5517000] offset:32
```

### zram -z 'zram addr'
Display all zspage of specified zram by zram address
```
crash> zram -z ffffff801530dc00
zspage[00001]:ffffff805414f400 class:1   fullness:1 pages:3 inuse:66    freeobj:66
zspage[00002]:ffffff8067f162b0 class:1   fullness:2 pages:3 inuse:255   freeobj:101
zspage[00003]:ffffff806578c780 class:1   fullness:2 pages:3 inuse:255   freeobj:235
zspage[00004]:ffffff8068ca4240 class:1   fullness:3 pages:3 inuse:256   freeobj:2147483647
zspage[00005]:ffffff8067fd7780 class:1   fullness:3 pages:3 inuse:256   freeobj:2147483647
zspage[00006]:ffffff8069481470 class:1   fullness:3 pages:3 inuse:256   freeobj:2147483647
zspage[00007]:ffffff8067b21d30 class:1   fullness:3 pages:3 inuse:256   freeobj:2147483647
```

### zram -o 'size_class/zspage/page addr'
Display all obj info by zs_class/zspage/page address
```
crash> zram -o ffffff805414f400
           obj[000000]b8ae2000~b8ae2030 handle:ffffff8068ea93d0 index:0    alloc
           obj[000001]b8ae2030~b8ae2060 handle:ffffff8068d795e0 index:1    alloc
           obj[000002]b8ae2060~b8ae2090 handle:ffffff8013df55e0 index:2    alloc
           obj[000003]b8ae2090~b8ae20c0 handle:ffffff806b06ccd0 index:3    alloc
           obj[000004]b8ae20c0~b8ae20f0 handle:ffffff8068e612e0 index:4    alloc
           obj[000005]b8ae20f0~b8ae2120 handle:ffffff805b884520 index:5    alloc
```
## slub
This command dumps the slab info.

### slub -a
Display all slab info
```
crash> slub -a
kmem_cache:0xe2aeb880 adreno_dispatch_job
   kmem_cache_node:0xe2b96040 nr_partial:4 nr_slabs:4 total_objects:84
       slab:0xec3b65a0 order:0 VA:[0xc4c28000~0xc4c29000] totalobj:21 inuse:0 freeobj:21
           obj[00001]VA:[0xc4c28000~0xc4c280c0] status:freed
           obj[00002]VA:[0xc4c280c0~0xc4c28180] status:freed
           obj[00003]VA:[0xc4c28180~0xc4c28240] status:freed
           obj[00004]VA:[0xc4c28240~0xc4c28300] status:freed
```

### slub -s
Display slab memory info
```
crash> slub -s
kmem_cache name                    slabs slab_size  per_slab_obj total_objs obj_size   pad_size align_size total_size
eb00c280 kmalloc-64                4792  8KB        25           119176     64B        64       320B     36.37MB
eb0e1fc0 vm_area_struct            3623  8KB        25           87247      136B       8        320B     26.63MB
eb00f400 kmalloc-4k                1127  32KB       2            2242       4KB        4096     12KB     26.27MB
eb0e1240 inode_cache               1202  16KB       24           26310      472B       8        656B     16.46MB
eb0edfc0 kernfs_node_cache         1990  8KB        30           59595      88B        8        272B     15.46MB
```

### slub -c 'cache addr'
Display specified slab info by kmem_cache addr
```
crash> slub -c eb0e1fc0
kmem_cache:0xeb0e1fc0 vm_area_struct
   kmem_cache_node:0xeb081080 nr_partial:960 nr_slabs:3623 total_objects:87247
       slab:0xec7481d8 order:1 VA:[0xde246000~0xde248000] totalobj:25 inuse:6 freeobj:19
           obj[00001]VA:[0xde246000~0xde246140] status:freed
           obj[00002]VA:[0xde246140~0xde246280] status:freed
           obj[00003]VA:[0xde246280~0xde2463c0] status:freed
           obj[00004]VA:[0xde2463c0~0xde246500] status:freed
```

### slub -p
Check all slab cache poison info
```
crash> slub -p
kmem_cache_name           Poison_Result
kmalloc-64                PASS
vm_area_struct            PASS
kmalloc-4k                PASS
```

### slub -P 'cache addr'
Check specified slab cache poison info
```
crash> slub -P eb0e1fc0
kmem_cache_name           Poison_Result
vm_area_struct            PASS
```

### slub -l
Display the all of slub trace, include alloc and free
```
crash> slub -l
stack_id:2132518711 Allocated:56725 times kmem_cache:avtab_node size:10.82MB
   [<c04fa5b0>] avtab_insert_node+2c
   [<c04fb20c>] avtab_insertf+174
   [<c04faebc>] avtab_read_item+3ac
   [<c04fb004>] avtab_read+98
   [<c04fbf48>] policydb_read+568
   [<c0502538>] security_load_policy+d0
   [<c04f5720>] sel_write_load+138
   [<c0300858>] vfs_write+c4
   [<c0300a58>] ksys_write+68
   [<c0101000>] __hyp_idmap_text_end+0
```
### slub -t A
Display the all alloc trace.
```
crash> slub -t A
stack_id:2132518711 Allocated:56725 times kmem_cache:avtab_node size:10.82MB
   [<c04fa5b0>] avtab_insert_node+2c
   [<c04fb20c>] avtab_insertf+174
   [<c04faebc>] avtab_read_item+3ac
   [<c04fb004>] avtab_read+98
   [<c04fbf48>] policydb_read+568
   [<c0502538>] security_load_policy+d0
   [<c04f5720>] sel_write_load+138
   [<c0300858>] vfs_write+c4
   [<c0300a58>] ksys_write+68
   [<c0101000>] __hyp_idmap_text_end+0
```
### slub -t F
Display the all free trace.
```
crash> slub -t F
stack_id:3391341428 Freed:5743 times kmem_cache:vm_area_struct size:1.75MB
   [<c02c5b00>] exit_mmap+f4
   [<c0120aa8>] __mmput+1c
   [<c0129010>] do_exit+3a8
   [<c01297e4>] do_group_exit+40
   [<c0135488>] get_signal+21c
   [<c010b2f8>] do_work_pending+16c
   [<c010106c>] slow_work_pending+c
```
### slub -T 'stack_id'
Display the slub trace by given the stack_id.
```
crash> slub -T 866893361
Pid       Freq      Size
1         424       132.50KB
282       162       50.62KB
284       153       47.81KB
322       59        18.44KB
323       96        30KB
324       93        29.06KB
325       98        30.62KB
```
## dts
This command dumps the dts info.

### dts -a
Display whole dts info.
```
crash> dts -a
/{
        model=<standalone V1.0>;
        compatible=<,xx>;
        ,msm-id=< 1e6 10000 >;
        interrupt-parent=< 1 >;
        #address-cells=< 2 >;
        #size-cells=< 2 >;
        ,board-id=< 10022 1 >;
        name=<>;

        memory{
                ddr_device_hbb_ch0_rank0=< d >;
                ddr_device_rank_ch0=< 1 >;
                ddr_device_type=< 7 >;
                device_type=<memory>;
                reg=< 0 40000000 0 3f680000 >;
                name=<memory>;
        };
```
### dts -f
Display whole dts info with address.
```
crash> dts -f
0xf7b55db8:/{
        0xf7b55e64:model=<standalone V1.0>;
        0xf7b55e90:compatible=<xx>;
        0xf7b55ebc:,msm-id=< 1e6 10000 >;
        0xf7b55ee8:interrupt-parent=< 1 >;
        0xf7b55f14:#address-cells=< 2 >;
        0xf7b55f40:#size-cells=< 2 >;
        0xf7b55f6c:,board-id=< 10022 1 >;
        0xf7b55f98:name=<>;

        0xf7b55fc8:memory{
                0xf7b56078:ddr_device_hbb_ch0_rank0=< d >;
                0xf7b560a4:ddr_device_rank_ch0=< 1 >;
                0xf7b560d0:ddr_device_type=< 7 >;
                0xf7b560fc:device_type=<memory>;
                0xf7b56128:reg=< 0 40000000 0 3f680000 >;
                0xf7b56154:name=<memory>;
        };
```

### dts -s 'node name'
Display one node info by node name or node path.
```
crash> dts -s memory
/soc/memory@045f0000
0xf7b79248:memory@045f0000{
        0xf7b79300:compatible=<,rpm-msg-ram>;
        0xf7b7932c:reg=< 45f0000 4000 >;
        0xf7b79358:phandle=< 4e >;
        0xf7b79384:name=<memory>;
};

/memory
0xf7b55fc8:memory{
        0xf7b56078:ddr_device_hbb_ch0_rank0=< d >;
        0xf7b560a4:ddr_device_rank_ch0=< 1 >;
        0xf7b560d0:ddr_device_type=< 7 >;
        0xf7b560fc:device_type=<memory>;
        0xf7b56128:reg=< 0 40000000 0 3f680000 >;
        0xf7b56154:name=<memory>;
};
```

### dts -m
Display physic memory config.
```
crash> dts -m
┌────────────────────────────────────────────────────────────┐
│                   DDR MEMORY RANGES                        │
├─────┬────────────────────┬────────────────────┬────────────┤
│ No. │    Start Address   │     End Address    │    Size    │
├─────┼────────────────────┼────────────────────┼────────────┤
│   1 │ 0x0000000080e00000 │ 0x00000000817fffff │       10MB │
│   2 │ 0x0000000081cf5000 │ 0x0000000081cfefff │       40KB │
│   3 │ 0x0000000081f20000 │ 0x000000008249ffff │     5.50MB │
│   4 │ 0x0000000082800000 │ 0x000000009927ffff │   362.50MB │
│   5 │ 0x000000009ea9c000 │ 0x000000009eafffff │      400KB │
│   6 │ 0x000000009f300000 │ 0x00000000a63fffff │      113MB │
│   7 │ 0x00000000a7000000 │ 0x00000000e05fffff │      918MB │
│   8 │ 0x00000000e0a00000 │ 0x00000000e88fffff │      127MB │
│   9 │ 0x00000000ea700000 │ 0x00000000fc7fffff │      289MB │
│  10 │ 0x00000000fca00000 │ 0x00000000ffffffff │       54MB │
│  11 │ 0x0000000880000000 │ 0x00000008afbfefff │   764.00MB │
│  12 │ 0x00000008b0000000 │ 0x00000008ba6fffff │      167MB │
│  13 │ 0x00000008bf800000 │ 0x00000008bfffffff │        8MB │
│  14 │ 0x00000008c0000000 │ 0x000000097fffffff │        3GB │
├─────┴────────────────────┴────────────────────┴────────────┤
│ Total Memory Size: 5.75GB                                  │
└────────────────────────────────────────────────────────────┘
```

### dts -b 'file path'
Save the whole dtb memory to file.
```
crash> dts -b ./dts.dtb
dtb addr:ff8b2000, size:311582
save dtb to ./dts.dtb
```

## pageowner
This command dumps the pageowner info.

### pageowner -a
Display alloc stack for every page.
```
crash> pageowner -a
═══════════════════════════════════════════════════════════════
                    ALLOCATED PAGES SUMMARY
═══════════════════════════════════════════════════════════════
Total allocated entries: 380953
Total allocated pages:   429866
Total allocated memory:  1.64GB
═══════════════════════════════════════════════════════════════
[1/380953] ALLOC: PFN:0xbffff~0xc0000 (1 page, 4KB) Page:0xfffffffe01ffffc0 PID:1881 [CachedAppOptimi] 00:01:18.473104509 (uptime)
[<ffffffd4d55b039c>] post_alloc_hook+20c
[<ffffffd4d55b3064>] prep_new_page+28
[<ffffffd4d55b46a4>] get_page_from_freelist+12ac
[<ffffffd4d55b320c>] __alloc_pages+d8
[<ffffffd4d34bfb64>] zs_malloc+1c8
[<ffffffd4d37538e4>] zram_bvec_rw+2a8
[<ffffffd4d375340c>] zram_rw_page.4e8b0154c58fc8baa75c3124f9a25b1c+9c
[<ffffffd4d58ddba4>] bdev_write_page+88
[<ffffffd4d55c21cc>] __swap_writepage+64
[<ffffffd4d55c2120>] swap_writepage+50
[<ffffffd4d5555dbc>] shrink_page_list+d18
[<ffffffd4d5556cb0>] reclaim_pages+1fc
[<ffffffd4d55c132c>] madvise_cold_or_pageout_pte_range.50c4f95024e08bb75653a011da8190a2+79c
[<ffffffd4d55a4f44>] walk_pgd_range+324
[<ffffffd4d55a4a34>] walk_page_range+1cc
[<ffffffd4d55bfe40>] madvise_vma_behavior.50c4f95024e08bb75653a011da8190a2+900
```

### pageowner -f
Display free stack for every page
```
crash> pageowner -f
═══════════════════════════════════════════════════════════════
                      FREED PAGES SUMMARY
═══════════════════════════════════════════════════════════════
Total freed entries: 14
Total freed pages:   14
Total freed memory:  56KB
═══════════════════════════════════════════════════════════════
[1/14] FREE: PFN:0x58050~0x58051 (1 page, 4KB) Page:0xfffffffe00601400 PID:499 [lmkd] 00:00:11.878516285 (uptime)
[<ffffffd4d55b11a0>] free_unref_page_prepare+2d8
[<ffffffd4d55b1634>] free_unref_page_list+a0
[<ffffffd4d554e808>] release_pages+510
[<ffffffd4d55c3988>] free_pages_and_swap_cache+4c
[<ffffffd4d55a1678>] tlb_finish_mmu+8c
[<ffffffd4d55a03e8>] exit_mmap+130
[<ffffffd4d532f74c>] __mmput+38
[<ffffffd4d532f6e4>] mmput+30
[<ffffffd4d533b180>] do_exit+30c
[<ffffffd4d533bc88>] do_group_exit+98
[<ffffffd4d533bca8>] __wake_up_parent+0
[<ffffffd4d52b6ad4>] invoke_syscall+5c
[<ffffffd4d52b6a08>] el0_svc_common+c4
[<ffffffd4d52b6a6c>] do_el0_svc_compat+1c
[<ffffffd4d625ec24>] el0_svc_compat+30
[<ffffffd4d625ebc8>] el0t_32_sync_handler+60
```
### pageowner -t
Display the alloc memory size for every stack.
```
Allocated 127522 times, Total memory: 498.13MB
[<ffffffd4d55b039c>] post_alloc_hook+20c
[<ffffffd4d55b3064>] prep_new_page+28
[<ffffffd4d55b46a4>] get_page_from_freelist+12ac
[<ffffffd4d55b320c>] __alloc_pages+d8
[<ffffffd4d52cc40c>] alloc_zeroed_user_highpage_movable+38
[<ffffffd4d5598568>] handle_pte_fault+494
[<ffffffd4d5594820>] do_handle_mm_fault+4a0
[<ffffffd4d55898cc>] __get_user_pages+1cc
[<ffffffd4d5589c88>] __mm_populate+134
[<ffffffd4d559b038>] do_mlock+240
[<ffffffd4d559a700>] __arm64_sys_mlock+1c
[<ffffffd4d52b6ad4>] invoke_syscall+5c
[<ffffffd4d52b6a08>] el0_svc_common+c4
[<ffffffd4d52b6a6c>] do_el0_svc_compat+1c
[<ffffffd4d625ec24>] el0_svc_compat+30
[<ffffffd4d625ebc8>] el0t_32_sync_handler+60
-------------------------------------------------
PID      Comm                 Times      Size
3772     memtester            127522     498.13MB
```
### pageowner -n pfn/PA/VA
Display the alloc and free stack for specific pfn/PA/VA.
```
crash> pageowner -n 0xbfffe
Detected as PFN: 0xbffff
═══════════════════════════════════════════════════════════════
                    PAGE OWNER INFO FOR PFN:0xbffff
═══════════════════════════════════════════════════════════════
PFN Range      :   0xbffff - 0xbffff
Page Address   :   0xfffffffe01ffffc0
Order          :   0 (1 page, 4KB)
Current Status :   ALLOCATED
═══════════════════════════════════════════════════════════════
ALLOCATION HISTORY:
Status     :   ALLOC
PID        :   1881 [CachedAppOptimi]
Timestamp  :   00:01:18.473104509 (uptime)
GFP Mask   :   0x810280a
[<ffffffd4d55b039c>] post_alloc_hook+20c
[<ffffffd4d55b3064>] prep_new_page+28
[<ffffffd4d55b46a4>] get_page_from_freelist+12ac
[<ffffffd4d55b320c>] __alloc_pages+d8
[<ffffffd4d34bfb64>] zs_malloc+1c8
[<ffffffd4d37538e4>] zram_bvec_rw+2a8
[<ffffffd4d375340c>] zram_rw_page.4e8b0154c58fc8baa75c3124f9a25b1c+9c
[<ffffffd4d58ddba4>] bdev_write_page+88
[<ffffffd4d55c21cc>] __swap_writepage+64
[<ffffffd4d55c2120>] swap_writepage+50
[<ffffffd4d5555dbc>] shrink_page_list+d18
[<ffffffd4d5556cb0>] reclaim_pages+1fc
[<ffffffd4d55c132c>] madvise_cold_or_pageout_pte_range.50c4f95024e08bb75653a011da8190a2+79c
[<ffffffd4d55a4f44>] walk_pgd_range+324
[<ffffffd4d55a4a34>] walk_page_range+1cc
[<ffffffd4d55bfe40>] madvise_vma_behavior.50c4f95024e08bb75653a011da8190a2+900

FREE HISTORY:
Status     :   FREE
PID        :   1881 [CachedAppOptimi]
Timestamp  :   00:01:18.470217478 (uptime)
GFP Mask   :   0x810280a
[<ffffffd4d55b11a0>] free_unref_page_prepare+2d8
[<ffffffd4d55b1634>] free_unref_page_list+a0
[<ffffffd4d5556528>] shrink_page_list+1484
[<ffffffd4d5556cb0>] reclaim_pages+1fc
[<ffffffd4d55c132c>] madvise_cold_or_pageout_pte_range.50c4f95024e08bb75653a011da8190a2+79c
[<ffffffd4d55a4f44>] walk_pgd_range+324
[<ffffffd4d55a4a34>] walk_page_range+1cc
[<ffffffd4d55bfe40>] madvise_vma_behavior.50c4f95024e08bb75653a011da8190a2+900
[<ffffffd4d55bf354>] do_madvise+168
[<ffffffd4d55c015c>] __arm64_sys_process_madvise+150
[<ffffffd4d52b6ad4>] invoke_syscall+5c
[<ffffffd4d52b69d8>] el0_svc_common+94
[<ffffffd4d52b6a6c>] do_el0_svc_compat+1c
[<ffffffd4d625ec24>] el0_svc_compat+30
[<ffffffd4d625ebc8>] el0t_32_sync_handler+60
[<ffffffd4d5211d08>] el0t_32_sync+1b8
═══════════════════════════════════════════════════════════════
```
### pageowner -m
Display the alloc memory size for every process.
```
crash> pageowner -m
PID      Comm                 Times      Size
         page_owner                      20.80MB
         stack_record                    16KB
3772     memtester            179573     701.46MB
1        init                 19078      104.62MB
712      main                 14640      58.88MB
1881     CachedAppOptimi      13552      52.98MB
68       kswapd0              11550      45.20MB
960      unknow               8262       32.44MB
4268     ndroid.settings      6485       25.36MB
```
## rtb
This command is used to view detailed information about rtb log.

### rtb -a
Display all rtb log.
```
crash> rtb -a
[234.501572] [12532244321] <0>: LOGK_CTXID ctxid:4284 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501602] [12532244897] <0>: LOGK_IRQ interrupt:1 handled from addr ffffffd4d627c7b4 ipi_handler.04f2cb5359f849bb5e8105832b6bf932.cfi_jt Line 888 of "arch/arm64/ke            rnel/entry.S"
[234.501628] [12532245392] <0>: LOGK_CTXID ctxid:943 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501799] [12532248689] <0>: LOGK_CTXID ctxid:4284 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501829] [12532249254] <0>: LOGK_CTXID ctxid:1621 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501836] [12532249398] <0>: LOGK_IRQ interrupt:1 handled from addr ffffffd4d627c7b4 ipi_handler.04f2cb5359f849bb5e8105832b6bf932.cfi_jt Line 888 of "arch/arm64/ke            rnel/entry.S"
```

## wdt
This command is used to view watchdog information for watchdog implementations.

### wdt -a
Display watchdog info (automatically detects watchdog).

**Watchdog Example:**
```
crash> wdt -a
==== Watchdog Information ====
  enabled                  : True
  base                     : 0xc5a66000
  user_pet_enabled         : False
  pet_time                 : 9.36s
  bark_time                : 15s
  bite_time                : 18s
  bark_irq                 : 25
  user_pet_complete        : True
  wakeup_irq_enable        : True
  in_panic                 : True
  irq_ppi                  : False
  freeze_in_progress       : False
  jiffies                  : 4299314482
  last_jiffies_update      : 14790.61852094s
  tick_next_period         : 14790.62185427s
  tick_do_timer_cpu        : 3

pet_timer:
  expires                  : 4348153
  timer_expired            : False
  timer_fired              : 14784.48016105s

watchdog_thread:
  watchdog_task            : 0xeb1d5c80
  pid                      : 40
  cpu                      : 0
  task_state               : IN
  last_run                 : 14784.48038923s
  thread_start             : 14784.48063001s
  last_pet                 : 14784.48064095s
  do_ipi_ping              : True
  ping_cpu[0]              : 1203.19879500s ~ 1203.20327400s (4479ns)
  ping_cpu[1]              : 0.00000000s ~ 0.00000000s (0ns)
  ping_cpu[2]              : 0.00000000s ~ 0.00000000s (0ns)
  ping_cpu[3]              : 0.00000000s ~ 0.00000000s (0ns)

[Note] Pet timer has not triggered!
```

### rtb -c 'cpu'
Display rtb log with cpu.
```
crash> rtb -c 0
[234.501572] [12532244321] <0>: LOGK_CTXID ctxid:4284 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501602] [12532244897] <0>: LOGK_IRQ interrupt:1 handled from addr ffffffd4d627c7b4 ipi_handler.04f2cb5359f849bb5e8105832b6bf932.cfi_jt Line 888 of "arch/arm64/ke            rnel/entry.S"
[234.501628] [12532245392] <0>: LOGK_CTXID ctxid:943 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501799] [12532248689] <0>: LOGK_CTXID ctxid:4284 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501829] [12532249254] <0>: LOGK_CTXID ctxid:1621 called from addr ffffffd4d628a684 __schedule Line 220 of "include/trace/events/sched.h"
[234.501836] [12532249398] <0>: LOGK_IRQ interrupt:1 handled from addr ffffffd4d627c7b4 ipi_handler.04f2cb5359f849bb5e8105832b6bf932.cfi_jt Line 888 of "arch/arm64/ke            rnel/entry.S"
```
## wq
This command dumps the workqueue info.

### wq -w
Display all worker.
```
crash> wq -w
worker           name          pid    flags                                  workqueue       sleeping last_active IDLE last_func                                                          current_func
ffffff8017efbe40 kworker/u8:4  385    WORKER_PREP|WORKER_IDLE|WORKER_UNBOUND memlat_wq       0        4294951343  Yes  memlat_update_work.ced2387b2ffe06d71748357bedbe93a2.cfi_jt+0x0     None
ffffff8016d1e180 kworker/u8:3  362    WORKER_PREP|WORKER_IDLE|WORKER_UNBOUND adb             0        4294951343  Yes  ffs_user_copy_worker.b0cd75eaae5d58bcb6aaf6fa4f4857ee.cfi_jt+0x0   None
ffffff80083d9c00 kworker/2:1H  109    WORKER_PREP|WORKER_IDLE                kverityd        0        4294951341  Yes  verity_prefetch_io.bb921613a07c200788484c1e05cde4c1.cfi_jt+0x0     None
ffffff80697d3780 kworker/1:6H  3888   WORKER_PREP|WORKER_IDLE                kverityd        0        4294951341  Yes  verity_work.bb921613a07c200788484c1e05cde4c1.cfi_jt+0x0            None
ffffff8016f0b300 kworker/1:4   541    WORKER_PREP|WORKER_IDLE                events          0        4294951331  Yes  handle_update.f3ba35adcfc3b8df74da779354a74767.cfi_jt+0x0          None
ffffff80143c7a80 kworker/0:7   439    WORKER_PREP|WORKER_IDLE                pm              0        4294951325  Yes  pm_runtime_work.e82816fbe6e30b4c36613b999953c187.cfi_jt+0x0        None
ffffff801d246540 kworker/0:4H  1081   WORKER_PREP|WORKER_IDLE                kverityd        0        4294951320  Yes  verity_prefetch_io.bb921613a07c200788484c1e05cde4c1.cfi_jt+0x0     None
ffffff80194e7300 kworker/2:3   451    WORKER_PREP|WORKER_IDLE                mm_percpu_wq    0        4294951256  Yes  vmstat_update.e559268912d9db5a285c4a36ab11201a.cfi_jt+0x0          None
ffffff8013cad9c0 kworker/3:3   367    WORKER_PREP|WORKER_IDLE                mm_percpu_wq    0        4294951248  Yes  vmstat_update.e559268912d9db5a285c4a36ab11201a.cfi_jt+0x0          None
```
### wq -p
Display worker_pool.
```
crash> wq -p
worker_pool           cpu        workers    idle       running    works      flags
ffffff800301a400      Unbound    8          8          0          0          POOL_DISASSOCIATED
ffffff806d1b8580      1          15         15         0          0          None
ffffff806d1d6580      2          3          3          0          0          None
ffffff806d1f4580      3          3          3          0          0          None
```

### wq -P 'worker_pool addr'
Display worker by given the worker_pool address.
```
crash> wq -P ffffff800301a400
worker:
   kworker/u8:0 [Idle] pid:8
   kworker/u8:1 [Idle] pid:10
   kworker/u8:2 [Idle] pid:67
   kworker/u8:3 [Idle] pid:362
   kworker/u8:4 [Idle] pid:385
   kworker/u8:5 [Idle] pid:944
   kworker/u8:6 [Idle] pid:1530
   kworker/u8:7 [Idle] pid:1589

Delayed Work:

Pending Work:
```

## df
This command dumps the mount info.

### df -s
```
crash> df -s
Partition                      Type  Blocks     Block SZ   Size       Used       Avail      Use%
/system_dlkm                   ext4  3043       4KB        11.89MB    11.85MB    40KB       99.67%
/vendor/dsp                    ext4  15093      4KB        58.96MB    8.04MB     50.92MB    13.64%
/product                       ext4  46313      4KB        180.91MB   180.37MB   556KB      99.70%
/vendor                        ext4  81110      4KB        316.84MB   315.85MB   1012KB     99.69%
/                              ext4  186571     4KB        728.79MB   726.56MB   2.23MB     99.69%
/mnt/pass_through/0/emulated   f2fs  1834496    4KB        7.00GB     250.70MB   6.75GB     3.50%
/vendor_dlkm                   ext4  15286      4KB        59.71MB    59.53MB    184KB      99.70%
/mnt/vendor/persist            ext4  6908       4KB        26.98MB    908KB      26.10MB    3.29%
/metadata                      f2fs  15872      4KB        62MB       34.14MB    27.86MB    55.07%
/system_ext                    ext4  99458      4KB        388.51MB   387.33MB   1.18MB     99.70%
```
## binder
This command dumps the binder information.

### binder -a
Display all binder proc states.
```
crash> binder -a
binder_proc:0xffffff801a7a5400 ndroid.settings [4268] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:4 requested:0 started:2 ready:3
  binder_thread:0xffffff80543a5800 pid:4283 loop:18 need_return:0
  binder_thread:0xffffff802b9dbe00 pid:4268 loop:16 need_return:0
    outgoing transaction:0xffffff8068d7fa00 id:98800 from 4268:4268 to 504:504 code:7 flags:16 pri:SCHED_NORMAL[110] reply:1
  binder_thread:0xffffff8053816c00 pid:4284 loop:17 need_return:0
  binder_thread:0xffffff8062513400 pid:4285 loop:17 need_return:0
  binder_node:0xffffff8014417c00 id:98764 ptr:0xf1b053a0 cookie:0xefb79838 pri:SCHED_NORMAL[139] hs:1 hw:1 ls:0 lw:0 is:1 iw:1 tr:0
     binder_ref:0xffffff8066bd9600 id:98765 binder_proc:0xffffff8016b5ac00 system_server[1067]
```
### binder -p 'pid'
Display specific process binder states.
```
crash> binder -p 1067
binder_proc:0xffffff8017fd9400 system_server [1067] hwbinder dead:0 frozen:0 sr:0 ar:0 max:4 total:12 requested:0 started:4 ready:5
  binder_thread:0xffffff804d132000 pid:2046 loop:0 need_return:0
  binder_thread:0xffffff8058c4b600 pid:2103 loop:17 need_return:0
  binder_thread:0xffffff805c4aa400 pid:2619 loop:0 need_return:0
  binder_node:0xffffff8056dbc780 id:4259 ptr:0xe28965f0 cookie:0xe289c000 pri:SCHED_NORMAL[120] hs:1 hw:1 ls:0 lw:0 is:1 iw:1 tr:0
     binder_ref:0xffffff8056ec2280 id:4260 binder_proc:0xffffff80155e0c00 powerstateservi[588]
  binder_node:0xffffff805a50cf00 id:11867 ptr:0xed6eff10 cookie:0xde8d6150 pri:SCHED_NORMAL[120] hs:1 hw:1 ls:0 lw:0 is:1 iw:1 tr:0
     binder_ref:0xffffff8056f18080 id:11868 binder_proc:0xffffff8017652800 mediaswcodec[1184]
  binder_ref:0xffffff8027197a80 id:11301 desc:7 s:1 w:1 death:0 -> node_id:1423 binder_proc:0xffffff801436f000 vendor.qti.medi[908]
  binder_ref:0xffffff8053718c80 id:4199 desc:3 s:1 w:1 death:0xffffff8054f70540 -> node_id:1821 binder_proc:0xffffff8017de8c00 aonutility@1.0-[884]
  binder_ref:0xffffff8053718900 id:4152 desc:1 s:0 w:1 death:0 -> node_id:4151 binder_proc:0xffffff8016ba6c00 hwservicemanage[509]
```
### binder -l
Display binder transaction log.
```
crash> binder -l
98799 : reply from 1067:4161 to 4268:4268 context binder node 0 handle -1 size 1716:0 ret 0/0 l=0
98800 : call  from 4268:4268 to 504:0 context binder node 1 handle 0 size 156:0 ret 0/0 l=0
98737 : reply from 1067:4161 to 4268:4268 context binder node 0 handle -1 size 1716:0 ret 0/0 l=0
98738 : call  from 4268:4268 to 504:0 context binder node 1 handle 0 size 88:0 ret 0/0 l=0
98739 : reply from 504:504 to 4268:4268 context binder node 0 handle -1 size 32:8 ret 0/0 l=0
98741 : call  from 4268:4268 to 1067:0 context binder node 10747 handle 26 size 132:0 ret 0/0 l=0
98742 : reply from 1067:4161 to 4268:4268 context binder node 0 handle -1 size 8:0 ret 0/0 l=0
```

### binder -f
Display binder fail log.
```
crash> binder -f
45546 : call  from 3182:3182 to 0:0 context binder node 1066 handle 25 size 0:0 ret 29201/-1 l=3273
45551 : call  from 3182:3182 to 0:0 context binder node 1066 handle 25 size 0:0 ret 29201/-1 l=3273
58748 : call  from 3431:3431 to 0:0 context binder node 1066 handle 25 size 0:0 ret 29201/-1 l=3273
58753 : call  from 3431:3431 to 0:0 context binder node 1066 handle 25 size 0:0 ret 29201/-1 l=3273
68878 : async from 1067:1067 to 0:0 context binder node 0 handle 584 size 124:0 ret 29189/-22 l=3258
69422 : async from 1067:1067 to 0:0 context binder node 0 handle 588 size 124:0 ret 29189/-22 l=3258
77204 : async from 1067:2998 to 3719:0 context binder node 73053 handle 1017 size 84:0 ret 29189/-3 l=3445
```

### binder -n
Display binder node info.
```
crash> binder -n
binder_proc:0xffffff801a7a5400 ndroid.settings [4268] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:4 requested:0 started:2 ready:3
  binder_node:0xffffff8014417c00 id:98764 ptr:0xf1b053a0 cookie:0xefb79838 pri:SCHED_NORMAL[139] hs:1 hw:1 ls:0 lw:0 is:1 iw:1 tr:0
     binder_ref:0xffffff8066bd9600 id:98765 binder_proc:0xffffff8016b5ac00 system_server[1067]
  binder_node:0xffffff8053beea80 id:98727 ptr:0xf1b05240 cookie:0xefb797e8 pri:SCHED_NORMAL[139] hs:1 hw:1 ls:0 lw:0 is:1 iw:1 tr:0
     binder_ref:0xffffff80654e9980 id:98728 binder_proc:0xffffff8016b58400 perfservice[1130]
```

### binder -b
Display binder buffer info.
```
binder_proc:0xea194000 viders.calendar [7340] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:6 requested:0 started:4 ready:5
  binder_alloc:0xea194154 mm_struct:0xe681df80 vma:0xefa9d880 buffer:0x81a39000 size:1040384 free:520192
    Page :0xf815f28c PA:0xa664b000
    Page :0xf7d338f8 PA:0x88bce000
    Free binder_buffer :0xdb035300 id:409018 data:0x81a390e0 PA:0xa664b0e0 size:296 offset:24 extra:0 pid:666 delivered
    Alloc binder_buffer:0xdd8f62c0 id:408034 data:0x81a390d8 PA:0xa664b0d8 size:8 offset:0 extra:0 pid:1239 delivered

```

### binder -t
Display binder thread info.
```
crash> binder -t
binder_proc:0xffffff801a7a5400 ndroid.settings [4268] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:4 requested:0 started:2 ready:3
  binder_thread:0xffffff80543a5800 pid:4283 loop:18 need_return:0
  binder_thread:0xffffff802b9dbe00 pid:4268 loop:16 need_return:0
    outgoing transaction:0xffffff8068d7fa00 id:98800 from 4268:4268 to 504:504 code:7 flags:16 pri:SCHED_NORMAL[110] reply:1
  binder_thread:0xffffff8053816c00 pid:4284 loop:17 need_return:0
  binder_thread:0xffffff8062513400 pid:4285 loop:17 need_return:0
```
### binder -r
Display binder ref info.
```
crash> binder -r
binder_proc:0xffffff801a7a5400 ndroid.settings [4268] binder dead:0 frozen:0 sr:0 ar:0 max:15 total:4 requested:0 started:2 ready:3
  binder_ref:0xffffff80538f2000 id:97816 desc:7 s:1 w:1 death:0 -> node_id:7509 binder_proc:0xffffff8016b5ac00 system_server[1067]
  binder_ref:0xffffff80538f2880 id:97812 desc:3 s:1 w:1 death:0 -> node_id:4008 binder_proc:0xffffff8016b5ac00 system_server[1067]
  binder_ref:0xffffff805b026d00 id:97775 desc:1 s:1 w:1 death:0 -> node_id:5632 binder_proc:0xffffff8016b5ac00 system_server[1067]
  binder_ref:0xffffff805d210600 id:97772 desc:0 s:1 w:1 death:0 -> node_id:1 binder_proc:0xffffff801f8d6400 servicemanager[504]
  binder_ref:0xffffff80538f2480 id:97811 desc:2 s:1 w:1 death:0 -> node_id:4401 binder_proc:0xffffff8016b5ac00 system_server[1067]
```

## cpu
This command dumps the cpu info.

### cpu -p
Display cpu freq policy.
```
crash> cpu -p
CPU cpufreq_policy       cluster cur_freq   min_freq   max_freq   governor
0   e565fa00             0       1708800    1708800    1708800    schedutil
1   e565fa00             0       1708800    1708800    1708800    schedutil
2   e565fa00             0       1708800    1708800    1708800    schedutil
3   e565fa00             0       1708800    1708800    1708800    schedutil
```
### cpu -f
Display cpu freq table.
```
crash> cpu -f
CPU0            CPU1            CPU2            CPU3
614400          614400          614400          614400
864000          864000          864000          864000
1363200         1363200         1363200         1363200
1708800         1708800         1708800         1708800
```
### cpu -s
Display cpu state.
```
crash> cpu -s
cluster_data:0xffffffe595428460 Enable:true num_cpus:4 first_cpu:0 active_cpus:2
   cpu_data:0xffffff887a139de0 cpu:0 disabled:false is_busy:false busy_pct:32
   cpu_data:0xffffff887a35cde0 cpu:1 disabled:false is_busy:false busy_pct:50
   cpu_data:0xffffff887a57fde0 cpu:2 disabled:false is_busy:false busy_pct:1
   cpu_data:0xffffff887a7a2de0 cpu:3 disabled:false is_busy:false busy_pct:0

cluster_data:0xffffffe595428590 Enable:true num_cpus:1 first_cpu:4 active_cpus:1
   cpu_data:0xffffff887a9c5de0 cpu:4 disabled:false is_busy:false busy_pct:0
```

## tm
This command dumps the thermal info.

### tm -d
Display all thermal zone info.
```
crash> tm -d
ID    ADDR       Name               governor     cur_temp   last_temp
0     e9ad0400   pm5100-tz          step_wise    32086      32120
1     e9ad2800   pm5100-ibat-lvl0   step_wise    31         31
2     e9ad5800   pm5100-ibat-lvl1   step_wise    31         31
3     e9ad7000   pm5100-bcl-lvl0    step_wise    0          0
4     e9ae2800   pm5100-bcl-lvl1    step_wise    0          0
5     e9ae7000   pm5100-bcl-lvl2    step_wise    0          0
```
### tm -D
Display the temperature gear and cooling action of specified thermal zone.
```
crash> tm -D pm5100-tz
pm5100-tz:
   temperature:95000
      [3]thermal_cooling_device:0xe574e400 --> thermal-cpufreq-0
      [6]thermal_cooling_device:0xe574a800 --> cpu-isolate2
      [7]thermal_cooling_device:0xe574d800 --> cpu-isolate3
   temperature:115000
   temperature:145000
```
## dd
This command dumps the device driver info.

### dd -b
Display all bus info.
```
crash> dd -b
bus_type   name         subsys_private   probe func
c1c8dcd0   platform     eb1e0e00
c1c8dd88   cpu          eb1e0200
c1c8e068   container    eb1e1a00
c1c171b0   workqueue    eb1e6200
c1c543b8   gpio         eb1e5c00
c1c8e710   genpd        eb1e7400
c1c8f108   soc          eb1e7a00
c1c549c0   pci          ea0f0e00         pci_device_probe+0
```
### dd -B 'bus name'
Display all device and driver info for specified bus.
```
crash> dd -B platform
============================================================================
                                All devices
============================================================================
device     name                                                              driver     driver_name
ea0b4c10   reg-dummy                                                         c1c67ba8   reg-dummy
e9c1cc10   c600000.apps-smmu                                                 c1c7fc70   arm-smmu
============================================================================
                                All drivers
============================================================================
device_driver    name                            compatible                      probe func
c1ca4b50         bluesleep                       bcm,bluesleep                   platform_drv_probe+0
c1ca6110         lpm-levels                      ,lpm-levels                 platform_drv_probe+0
c1ca6f48         pwrseq_simple                   mmc-pwrseq-simple               platform_drv_probe+0
```
### dd -c
Display all class info.
```
crash> dd -c
class      name                 subsys_private
c1c678d0   regulator            eb1e0800
ea0d6680   bdi                  ea0f3800
c1c54478   gpio                 ea0f5000
c1c54850   pci_bus              ea0f6800
ea0d7680   backlight            ea0f4400
```
### dd -C 'class name'
Display all device info for specified class.
```
crash> dd -C backlight
============================================================================
                                All devices
============================================================================
device     name             driver     driver_name
e3174c60   panel0-backlight

```
### dd -l
Display all device info.
```
crash> dd -l
device     name                                                              Bus             driver     driver_name
ea0b4c10   reg-dummy                                                         platform        c1c67ba8   reg-dummy
ea112810   soc                                                               platform
ea110410   soc:psci                                                          platform
```
### dd -L
Display all driver info.
```
crash> dd -L
device_driver    name                                     Bus             compatible                      probe func
c1ca6f48         pwrseq_simple                            platform        mmc-pwrseq-simple               platform_drv_probe+0
c1ca6fd8         pwrseq_emmc                              platform        mmc-pwrseq-emmc                 platform_drv_probe+0
```
### dd -s
Display all device for specified driver.
```
crash> dd -s rpm-smd-regulator-resource
   device     name
   e9b44c10   soc:rpm-smd:rpm-regulator-smpa1
   e9001010   soc:rpm-smd:rpm-regulator-smpa3
   e9006410   soc:rpm-smd:rpm-regulator-smpa4
```
### dd -a
Display all char device.
```
crash> dd -a
char_device_struct   major      minorct    cdev               name
ffffff880acf0b00     510        8          0                  ttyGS
ffffff8802782680     1          256        ffffff88023670c0   mem
ffffff884a4f3880     256        256        ffffff884a65fb40   msm_rng
ffffff8806658200     511        1048576    0                  vfio
ffffff8802782b00     5          1          0                  /dev/tty
ffffff8802782e00     5          1          0                  /dev/console
```
### dd -m
Display all miscdevice.
```
crash> dd -m
miscdevice           minor      ops                            name
ffffff801c2ffa88     111        fastrpc_fops                   fastrpc-adsp-secure
ffffffe59814dca0     112        acc_fops                       usb_accessory
ffffff884041e498     115        tmc_fops                       tmc_etr1
ffffff884041b498     116        tmc_fops                       tmc_etr0
ffffff883a72cc98     117        tmc_fops                       tmc_etf0
```

### dd -D
Display all block device.
```
crash> dd -D
block_device      super_block       type  bsize devname         volname              UUID
ffffff8809154280  ffffff885d8d4000  ext4  4096  mmcblk0p1       xbl_a                0c768d03-3a3c-a990-0600-daf94674e882
ffffff8809156a40  ffffff885d32c000  ext4  4096  loop13          xbl_b                fcb4ebd9-7f11-c81c-ef48-43b90e410a24
ffffff8818e60d80  ffffff885d32c000  ext4  4096  mmcblk0p5       shrm_a               4717f043-65f9-3aa8-0e23-877e2c76ca7f
```

### dd -d
Display all disk.
```
crash> dd -d
gendisk            minor major partitions name
ffffff8802ffa000   1     1     1          ram1
ffffff8802ffb800   1     1     1          ram2
ffffff8802ffe800   1     1     1          ram0
```

### dd -p mmcblk0
Display all partition for specified disk.
```
crash> dd -p mmcblk0
block_device      partno start_sect sectors    size       bsize  type   devname              volname              UUID
ffffff8818e9a800  0      21364736   471040     230MB      0             mmcblk0p69           vm-persist           05bdafd5-4ec6-238c-7366-42ba33c98fb2
ffffff8818ea1ac0  0      30670848   512        256KB      0             mmcblk0p82           apdp                 dafea201-493e-7f88-ae4b-590a34946832
ffffff8818ea5d00  0      30539776   8          4KB        0             mmcblk0p81           devinfo              2e547183-c9aa-f37a-0bbc-84d081cc4394
```

## getprop
This command dumps the property info.

### getprop -s 'symbol path'
Load symbol for property，you must insmod the zram ko before run this command.
```
crash> mod -s zram ./lib/zram.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d3758440  zram                       ffffffd4d3752000     49152  ./lib/zram.ko

crash> mod -s zsmalloc ./lib/zsmalloc.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d34c6200  zsmalloc                   ffffffd4d34bf000     57344  ./lib/zsmalloc.ko

crash> getprop -s xxx/symbols
add symbol table from file "xxx/symbols/system/lib/bootstrap/libc.so"
Reading symbols from xxx/symbols/system/lib/bootstrap/libc.so...
Add symbol:xxx/symbols/system/lib/bootstrap/libc.so succ
```
### getprop -a
Display all propertys.
```
crash> getprop -a
[0001]persist.sys.timezone                                 GMT
[0006]dev.mnt.dev.data_mirror.ref_profiles                 dm-12
[0007]dev.mnt.dev.data_mirror.cur_profiles                 dm-12
[0008]dev.mnt.dev.system_dlkm                              dm-11
[0009]dev.mnt.dev.vendor_dlkm                              dm-10
[0010]dev.mnt.dev.vendor.bt_firmware                       mmcblk0p30
```
### getprop -p 'prop name'
Display specified property's value.
```
crash> getprop -p dev.mnt.dev.vendor.bt_firmware
mmcblk0p30
```
## logcat
This command dumps the logcat log info.

### logcat -s 'symbol path'
Load symbol for logd. you must insmod the zram ko before run this command.
```
crash> mod -s zram ./lib/zram.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d3758440  zram                       ffffffd4d3752000     49152  ./lib/zram.ko

crash> mod -s zsmalloc ./lib/zsmalloc.ko
     MODULE       NAME                             BASE            SIZE  OBJECT FILE
ffffffd4d34c6200  zsmalloc                   ffffffd4d34bf000     57344  ./lib/zsmalloc.ko

crash> logcat -s path
Add symbol table from file "path/libc.so"
Reading symbols from path/libc.so...
```
###  logcat -b 'log id'
Display all logcat log.
```
crash> logcat -b all
android_version is : 34 !
06-25 18:58:59.000534 499   499   1069   D lowmemorykiller Zone Normal: free:0 high:0 cma:0 reserve:(0 0 0) anon:(0 0) file:(0 0)
06-25 18:58:59.000534 499   499   1069   D lowmemorykiller Zone Movable: free:0 high:0 cma:0 reserve:(0 0 0) anon:(0 0) file:(0 0)
06-25 18:58:59.000534 499   499   1069   I lowmemorykiller Ignoring pressure since per-zone watermarks ok
06-25 18:58:59.000547 499   499   1069   I lowmemorykiller super critical memory pressure event is triggered
```
Display main log.
```
crash> logcat -b main
android_version is : 34 !
06-25 18:58:59.000534 499   499   1069   D lowmemorykiller Zone Normal: free:0 high:0 cma:0 reserve:(0 0 0) anon:(0 0) file:(0 0)
06-25 18:58:59.000534 499   499   1069   D lowmemorykiller Zone Movable: free:0 high:0 cma:0 reserve:(0 0 0) anon:(0 0) file:(0 0)
06-25 18:58:59.000534 499   499   1069   I lowmemorykiller Ignoring pressure since per-zone watermarks ok
06-25 18:58:59.000547 499   499   1069   I lowmemorykiller super critical memory pressure event is triggered
```
Display system log.
```
crash> logcat -b system
06-25 18:56:37.000327 1067  3459  1000   I ActivityManager Process android.process.acore (pid 2783) has died: prev LAST
06-25 18:56:37.000329 1067  2066  1000   D CountryDetector No listener is left
06-25 18:56:37.000338 1067  1634  1000   D DisplayManagerService Drop pending events for gone uid 10033
06-25 18:56:37.000893 1067  3459  1000   I ActivityManager Process android.process.media (pid 3002) has died: prev LAST
06-25 18:56:37.000898 1067  1634  1000   D DisplayManagerService Drop pending events for gone uid 10030
06-25 18:56:38.000342 2023  3579  10100  D PowerUI can't show warning due to - plugged: true status unknown: false
06-25 18:56:38.000346 2023  3580  10100  D PowerUI can't show warning due to - plugged: true status unknown: false
```
Display radio log.
```
crash> logcat -b radio
01-01 08:07:24.000061 839   894   1021   D RilApiSession [RilApiSession.cpp: 139] [Loc_hal_worker(839,894)] initialize: Initializing RIL API session.
01-01 08:07:24.000081 839   894   1021   D RilApiSession [RilApiSession.cpp: 52] [Loc_hal_worker(839,894)] createUnixSocket: Failed to connect to the server. Error: No such file or directory
01-01 08:07:26.000084 839   894   1021   D RilApiSession [RilApiSession.cpp: 139] [Loc_hal_worker(839,894)] initialize: Initializing RIL API session.
01-01 08:07:26.000084 839   894   1021   D RilApiSession [RilApiSession.cpp: 52] [Loc_hal_worker(839,894)] createUnixSocket: Failed to connect to the server. Error: No such file or directory
01-01 08:07:28.000085 839   894   1021   D RilApiSession [RilApiSession.cpp: 139] [Loc_hal_worker(839,894)] initialize: Initializing RIL API session.
01-01 08:07:28.000085 839   894   1021   D RilApiSession [RilApiSession.cpp: 52] [Loc_hal_worker(839,894)] createUnixSocket: Failed to connect to the server. Error: No such file or directory
01-01 08:07:31.000504 1065  1065  1001   D RILD **RIL Daemon Started**
01-01 08:07:31.000509 1065  1065  1001   D RILD **RILd param count=1**
```
Display event log.
```
crash> logcat -b events
01-01 08:07:10.000236 504   504   1000   I :[121035042,-1,]
01-01 08:07:10.000256 504   504   1000   I :[Multiple same specifications for vendor.qti.gnss.ILocAidlGnss/default.]
01-01 08:07:10.000256 504   504   1000   I :[Multiple same specifications for vendor.qti.hardware.qteeconnector.IAppConnector/default.]
01-01 08:07:10.000256 504   504   1000   I :[Multiple same specifications for vendor.qti.hardware.qteeconnector.IGPAppConnector/default.]
01-01 08:07:10.000259 504   504   1000   I :[SELinux: Loaded service context from:]
01-01 08:07:10.000259 504   504   1000   I :[           /system/etc/selinux/plat_service_contexts]
01-01 08:07:10.000259 504   504   1000   I :[           /system_ext/etc/selinux/system_ext_service_contexts]
01-01 08:07:10.000259 504   504   1000   I :[           /product/etc/selinux/product_service_contexts]
01-01 08:07:10.000259 504   504   1000   I :[           /vendor/etc/selinux/vendor_service_contexts]
01-01 08:07:10.000413 512   512   1000   I :[121035042,-1,]
01-01 08:00:00.000160 487   487   1036   I :[type=2000 audit(0.0:1): state=initialized audit_enabled=0 res=1]
01-01 08:07:02.000011 487   487   1036   I :[type=1403 audit(0.0:2): auid=4294967295 ses=4294967295 lsm=selinux res=1]
```
Display crash log.
```
crash> logcat -b crash
06-25 18:56:18.000253 3098  3098  10137  E AndroidRuntime FATAL EXCEPTION: main
Process: com.android.devicelockcontroller, PID: 3098
java.lang.ExceptionInInitializerError
        at com.android.devicelockcontroller.SystemDeviceLockManagerImpl.getInstance(SystemDeviceLockManagerImpl.java:68)
        at com.android.devicelockcontroller.policy.DevicePolicyControllerImpl.<init>(DevicePolicyControllerImpl.java:103)
        at com.android.devicelockcontroller.policy.DevicePolicyControllerImpl.<init>(DevicePolicyControllerImpl.java:89)
        at com.android.devicelockcontroller.DeviceLockControllerApplication.getPolicyController(DeviceLockControllerApplication.java:81)
        at com.android.devicelockcontroller.DeviceLockControllerApplication.getStateController(DeviceLockControllerApplication.java:74)
        at com.android.devicelockcontroller.DeviceLockControllerApplication.onCreate(DeviceLockControllerApplication.java:67)
        at android.app.Instrumentation.callApplicationOnCreate(Instrumentation.java:1317)
        at android.app.ActivityThread.handleBindApplication(ActivityThread.java:7017)
        at android.app.ActivityThread.-$$Nest$mhandleBindApplication(Unknown Source:0)
        at android.app.ActivityThread$H.handleMessage(ActivityThread.java:2237)
        at android.os.Handler.dispatchMessage(Handler.java:106)
        at android.os.Looper.loopOnce(Looper.java:205)
        at android.os.Looper.loop(Looper.java:294)
        at android.app.ActivityThread.main(ActivityThread.java:8223)
        at java.lang.reflect.Method.invoke(Native Method)
        at com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run(RuntimeInit.java:552)
        at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:977)
Caused by: java.lang.NullPointerException: Attempt to invoke virtual method 'android.devicelock.IDeviceLockService android.devicelock.DeviceLockManager.getService()' on a null object reference
        at com.android.devicelockcontroller.SystemDeviceLockManagerImpl.<init>(SystemDeviceLockManagerImpl.java:51)
        at com.android.devicelockcontroller.SystemDeviceLockManagerImpl.<init>(SystemDeviceLockManagerImpl.java:55)
        at com.android.devicelockcontroller.SystemDeviceLockManagerImpl.<init>(SystemDeviceLockManagerImpl.java:0)
        at com.android.devicelockcontroller.SystemDeviceLockManagerImpl$SystemDeviceLockManagerHolder.<clinit>(SystemDeviceLockManagerImpl.java:60)
        ... 17 more
```
## coredump
This command generate process coredump.

### coredump -p 'pid'
Generate process coredump.
```
crash> coredump -p 323
```

### coredump -p 'pid' -s 'symbols_path' -f
Generate process fake coredump with symbols.
```
overwrite /system/bin/logd:[size:0x15000 off:0] to core:[0x6052e20000 - 0x6052e35000]
```

### coredump -p 'pid' -s 'symbols_path' -r
```
overwrite PHDR /system/bin/logd:[size:0x15000 off:0] to core:[0x6052e20000 - 0x6052e35000]
```

### coredump -l 'pid' -s 'symbols_path'
get the linkmap with symbols
```
addr                ld                  next                prev                name
56ef6c4000          56ef6cc090          7eafbb4158          0                   /system/bin/app_process64
7eafa2a000          7eafb9f3b8          7eae8eb388          7eae8eb0e0          /system/bin/linker64
7eafa29000          7eafa29848          7eae8eb630          7eafbb4158          [vdso]
```

### coredump -m 'pid'
Show process maps.
```
crash> coredump -m 437
VMA:ffffff801d653b40 [6052e20000-6052e35000] r--p 0000000000000071 00000000 /system/bin/logd
VMA:ffffff801d653000 [6052e38000-6052ebd000] r-xp 0000000000000075 00000018 /system/bin/logd
VMA:ffffff80279c05a0 [7befacb000-7befb14000] rw-p 0000000000100073 07befacb [anon:scudo:secondary]
```

## tm
This command dumps the thermal info.

### tm -d
Display all thermal zone info.
```
crash> tm -d
ID    ADDR       Name               governor     cur_temp   last_temp
0     e9ad0400   pm5100-tz          step_wise    32086      32120
1     e9ad2800   pm5100-ibat-lvl0   step_wise    31         31
2     e9ad5800   pm5100-ibat-lvl1   step_wise    31         31
3     e9ad7000   pm5100-bcl-lvl0    step_wise    0          0
4     e9ae2800   pm5100-bcl-lvl1    step_wise    0          0
5     e9ae7000   pm5100-bcl-lvl2    step_wise    0          0
```
### tm -D 'zone name'
Display the temperature gear and cooling action of specified thermal zone.
```
crash> tm -D pm5100-tz
pm5100-tz:
   temperature:95000
      [0]thermal_cooling_device:0xffffff8009240800 --> cpufreq-cpu0
      [7]thermal_cooling_device:0xffffff801e78d800 --> thermal-pause-4
      [9]thermal_cooling_device:0xffffff801eafc800 --> thermal-pause-8
   temperature:115000
   temperature:145000
```

## cache
This command dumps the page cache info.

### cache -f
Display all file info.
```
crash> cache -f
Total cached files: 2951, total size: 427.85MB
===============================================
inode            address_space    nrpages  size       Path
ffffff880089cb68 ffffff880089cd30 5654     22.09MB    /lib/modules/6.1.115-android14-11-maybe-dirty-debug/kiwi_v2.ko
ffffff88044ef388 ffffff88044ef550 5503     21.50MB    /usr/lib/hw/camera.qcom.so
ffffff880bc64f08 ffffff880bc650d0 2660     10.39MB    /tmp/weston.log
ffffff8041b8fc68 ffffff8041b8fe30 2532     9.89MB     unknown
ffffff88012efa38 ffffff88012efc00 2062     8.05MB     /usr/lib/librsvg-2.so.2.48.0
ffffff803dfb9548 ffffff803dfb9710 2048     8MB        /run/log/journal/701d5770d3884580b981e8dad7fdc802/system.journal
ffffff8045363e88 ffffff8045364050 2048     8MB        /run/log/journal/0306e3733ef9480a8ef2e76eaa2834f9/system.journal
```

### cache -a
Display all anon pages.
```
crash> cache -a
page:0xfffffffe00000000  paddr:0x40000000
page:0xfffffffe00000040  paddr:0x40001000
page:0xfffffffe00000080  paddr:0x40002000
page:0xfffffffe000000c0  paddr:0x40003000
```
### cache -l
Display the file info of special path.
```
crash> cache -l /run/log/journal/0306e3733ef9480a8ef2e76eaa2834f9/
DENTRY             INODE              PERMISSIONS      UID      GID     SIZE   NRPAGE TIME         PATH
0xffffff8802487138 0xffffff8045363e88 -rw-r-----         0      995  8388608     2048 Jan 02 03:43 system.journal
0xffffff8802487c18 0xffffff8045362e08 -rw-r-----         0      995  8388608     1382 Jan 02 03:38 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000c340d-000000103eef2e0f.journal
0xffffff880261a740 0xffffff8802c03648 -rw-r-----         0      995  8388608     1382 Jan 02 03:22 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000c0990-0000001005f39a2b.journal
0xffffff8802486ae0 0xffffff8045364ae8 -rw-r-----         0      995  8388608     1382 Jan 02 03:06 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000bdf13-0000000fccca8480.journal
0xffffff880102b308 0xffffff804c559548 -rw-r-----         0      995  8388608     1382 Jan 02 02:51 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000bb496-0000000f939ee27e.journal
0xffffff8802487790 0xffffff8045360d08 -rw-r-----         0      995  8388608     1382 Jan 02 02:35 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b8a19-0000000f5a5e7b48.journal
0xffffff88011e13f0 0xffffff804bc89968 -rw-r-----         0      995  5658816     1382 Jan 02 02:19 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b5f9c-0000000f215ccc3c.journal
0xffffff8802487878 0xffffff8045367848 -rw-r-----         0      995  8388608     1484 Jan 02 02:03 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b32da-0000000ee9031143.journal
0xffffff880261a488 0xffffff8802c01d88 -rw-r-----         0      995  8388608     1381 Jan 02 01:47 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b08a6-0000000eb0889fb1.journal
0xffffff880261b308 0xffffff8802c01548 -rw-r-----         0      995  8388608     1382 Jan 02 01:31 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000ade28-0000000e77a80a2d.journal
0xffffff8802486658 0xffffff8045361548 -rw-r-----         0      995  8388608     1382 Jan 02 01:15 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000ab3aa-0000000e3ece08da.journal
0xffffff8802487d00 0xffffff8045363228 -rw-r-----         0      995  8388608     1382 Jan 02 00:59 system@bf00ed31f63c44bd8311c1e4185809e9-00000000000a892c-0000000e05db5bbd.journal
```
### cache -d
Dump the file content of special file.
```
crash> cache -d /run/log/journal/0306e3733ef9480a8ef2e76eaa2834f9/system.journal
Save system.journal to xxx/cache//run/log/journal/0306e3733ef9480a8ef2e76eaa2834f9/system.journal
  - File size: 8388608 bytes
  - Pages processed: 2048
  - Pages written: 2048
  - Pages excluded: 0
  - Success rate: 100.0%
```

## dbi
This command dumps debug image region information and manages CPU registers.

### dbi -a
Display memdump information.
```
crash> dbi -a
Id   Dump_entry       version  magic            DataAddr         DataLen    Name
0    bc707e10         20       42445953         bc707e50         2048       c0_context
1    bc708650         20       42445953         bc708690         2048       c1_context
2    bc708e90         20       42445953         bc708ed0         2048       c2_context
3    bc7096d0         20       42445953         bc709710         2048       c3_context
96   bc709f10         20       42445953         bc709f50         36928      l1_icache0
97   bc712f90         20       42445953         bc712fd0         36928      l1_icache1
98   bc71c010         20       42445953         bc71c050         36928      l1_icache2
```

### dbi -d
Parse CPU context and generate CMM files.
```
crash> dbi -d
c0_context  core:0  version:1.4
c1_context  core:1  version:1.4
c2_context  core:2  version:1.4
c3_context  core:3  version:1.4
Saved CMM files: /path/to/cmm/core0_regs.cmm and /path/to/cmm/secure_world_core0_regs.cmm
Saved CMM files: /path/to/cmm/core1_regs.cmm and /path/to/cmm/secure_world_core1_regs.cmm
Saved CMM files: /path/to/cmm/core2_regs.cmm and /path/to/cmm/secure_world_core2_regs.cmm
Saved CMM files: /path/to/cmm/core3_regs.cmm and /path/to/cmm/secure_world_core3_regs.cmm
```

### dbi -s
Print CPU stack traces for all cores from sdi register.
```
crash> dbi -s
c0_context  core:0  version:1.4
Core0
PC: <ffffffd4d52b5b54>: ipi_handler.04f2cb5359f849bb5e8105832b6bf932+c8
LR: <ffffffd4d52b5b4c>: ipi_handler.04f2cb5359f849bb5e8105832b6bf932+c0

PID: 504      TASK: ffffff8013aa4e00  CPU: 0    COMMAND: "servicemanager"
 #0 [ffffffc008003b60] handle_percpu_devid_irq at ffffffd4d53cd864
 #1 [ffffffc008003bb0] handle_domain_irq at ffffffd4d53c4f8c
 #2 [ffffffc008003bf0] gic_handle_irq.0baca5b50aed29204608b368989cedda at ffffffd4d6297afc
 #3 [ffffffc008003c20] do_interrupt_handler at ffffffd4d52a1140
 #4 [ffffffc008003c30] el1_interrupt at ffffffd4d625d9e4
 #5 [ffffffc008003c50] el1h_64_irq_handler at ffffffd4d625d9a8
 #6 [ffffffc008003d90] el1h_64_irq at ffffffd4d5211370
 #7 [ffffffc008003db0] _raw_spin_unlock_irqrestore at ffffffd4d62952c4
 #8 [ffffffc008003dd0] swake_up_one at ffffffd4d53a1a70
 #9 [ffffffc008003e10] rcu_report_qs_rnp at ffffffd4d53e64a0
#10 [ffffffc008003e70] rcu_core at ffffffd4d53eed94
#11 [ffffffc008003ed0] rcu_core_si.4b91badfc51da2dfae7c420c3dfdf5d0 at ffffffd4d53e72b4
#12 [ffffffc008003f10] _stext at ffffffd4d5210198
#13 [ffffffc008003f70] __irq_exit_rcu at ffffffd4d533fa68
#14 [ffffffc008003f90] irq_exit at ffffffd4d533fb0c
#15 [ffffffc008003fa0] handle_domain_irq at ffffffd4d53c4f90
#16 [ffffffc008003fe0] gic_handle_irq.0baca5b50aed29204608b368989cedda at ffffffd4d6297afc
--- <IRQ stack> ---
#17 [ffffffc00cf139f0] call_on_irq_stack at ffffffd4d521495c
#18 [ffffffc00cf13a10] do_interrupt_handler at ffffffd4d52a1120
#19 [ffffffc00cf13a20] el1_interrupt at ffffffd4d625d9e4
```

### dbi -i <cpu>
Print IRQ stack for cpu.
```
crash> dbi -i 2
CPU[0] irq stack:0xffffffc080000000~0xffffffc080004000
[0]Potential backtrace -> FP:0xffffffc080003ec0, LR:0xffffffc080003ec8
PID: 0        TASK: ffffffc0822dccc0  CPU: 0    COMMAND: "swapper/0"
 #0 [ffffffc080003ef0] sched_clock at ffffffc0801dc4dc
 #1 [ffffffc080003f10] sched_clock_cpu at ffffffc080148fec
 #2 [ffffffc080003f20] irqtime_account_irq at ffffffc0801318b8
 #3 [ffffffc080003f80] handle_softirqs at ffffffc0800d7624
 #4 [ffffffc080003fe0] __do_softirq at ffffffc080010200
 #5 [ffffffc080003ff0] ____do_softirq at ffffffc08001f230
--- <IRQ stack> ---
 #6 [ffffffc0822c3b80] call_on_irq_stack at ffffffc08001f1bc
 #7 [ffffffc0822c3bf0] init_thread_union at ffffffc0822c3bec
 #8 [ffffffc0822f9e20] __kvm_nvhe___kcfi_typeid_hyp_unpin_shared_mem at ffffff887ab592fc
```

### dbi -p <pid>
Print task stack for specified PID (ARM64 only).
```
crash> dbi -p 141
cpu_context:
   X19: 0xffffff8800523b40
   X20: 0xffffff8803cea780
   X21: 0xffffff8803cea780
   X22: 0
   X23: 0x402
   X24: 0xffffff8800523b40
   X25: 0x2
   X26: 0x1
   X27: 0xffffff8803cea780
   X28: 0
   fp:  0xffffffc083c0bd00
   sp:  0xffffffc083c0bd00
   pc:  0xffffffc081128a94

Stack:0xffffffc083c08000~0xffffffc083c0c000
[0]Potential backtrace -> FP:0xffffffc083c0bb80, LR:0xffffffc083c0bb88
 #0 [ffffffc083c0bba0] hrtimer_try_to_cancel at ffffffc0801c8060
 #1 [ffffffc083c0bbe0] dl_server_stop at ffffffc080133ca4
 #2 [ffffffc083c0bc40] dequeue_entities at ffffffc080123690
 #3 [ffffffc083c0bc80] psi_group_change at ffffffc0801531e4
 #4 [ffffffc083c0bce0] psi_task_switch at ffffffc0801536ac
 #5 [ffffffc083c0bd00] __switch_to at ffffffc08112897c
 #6 [ffffffc083c0bd80] __schedule at ffffffc0811295ac
 #7 [ffffffc083c0bde0] schedule at ffffffc081129b34
 #8 [ffffffc083c0be10] rescuer_thread at ffffffc0800fd118
 #9 [ffffffc083c0be70] kthread at ffffffc080102e20
```

### dbi -c <cpu> -l <cmm>
Load CPU registers from CMM file (ARM64 only).
```
crash> dbi -c 0 -l /path/to/core0_regs.cmm
CPU 0 registers loaded from /path/to/core0_regs.cmm
```

## ipc
This command dumps the ipc log.

### ipc -a
Display all ipc module info.
```
crash> ipc -a
ipc_log_context  Magic      Version first_page       last_page        write_page       read_page        Name
ffffff80039a3b00 25874452   3       ffffff8007b9c000 ffffff8007b9d000 ffffff8007b9c000 ffffff8007b9c000 glink_probe
ffffff8008ee5700 25874452   3       ffffff8008904000 ffffff8008900000 ffffff8008904000 ffffff8008904000 bcl_0x4700_0
ffffff80088a1800 25874452   3       ffffff8007bb7000 ffffff8007bb4000 ffffff8007bb4000 ffffff8007bb7000 smp2p
ffffff8012f27500 25874452   3       ffffff8008905000 ffffff8012fb7000 ffffff800a038000 ffffff800a038000 rpm-glink
ffffff8012f9ec00 25874452   3       ffffff8012fc6000 ffffff8012fcb000 ffffff8012fc3000 ffffff8012fc3000 qrtr_ns
ffffff8012e2a100 25874452   3       ffffff8003121000 ffffff8003126000 ffffff8003121000 ffffff8003121000 glink_ssr
ffffff80041c9700 25874452   3       ffffff8007bb1000 ffffff800216e000 ffffff8007bb1000 ffffff8007bb1000 mmc0
ffffff8018af1600 25874452   3       ffffff8018e8c000 ffffff8018e8c000 ffffff8018e8c000 ffffff8018e8c000 dma_bam_log
```

### ipc -l 'ipc module name'
Display ipc log.
```
crash> ipc -l bcl_0x4700_0
[ 1.748384640 0x722c8ab9b01]   [kworker/u8:1]: bcl_read_ibat: ibat:0 mA ADC:0x00
[ 1.748404692 0x722c8ab9c80]   [kworker/u8:1]: bcl_read_vbat_tz: vbat:4185 mv ADC:0x54
[ 1.748409536 0x722c8ab9cdd]   [kworker/u8:1]: bcl_read_lbat: LVLbat:5 val:0
[ 1.748486775 0x722c8aba2a8]   [kworker/u8:1]: bcl_read_ibat: ibat:0 mA ADC:0x00
[ 1.748504744 0x722c8aba401]   [kworker/u8:1]: bcl_read_vbat_tz: vbat:4185 mv ADC:0x54
[ 1.748508494 0x722c8aba449]   [kworker/u8:1]: bcl_read_lbat: LVLbat:6 val:0
[ 9.688728102 0x722d1c1e0c3]   [kworker/2:0H]: bcl_read_ibat: ibat:0 mA ADC:0x00
[ 9.688746748 0x722d1c1e227]   [kworker/2:0H]: bcl_read_vbat_tz: vbat:4185 mv ADC:0x54
[ 9.688751227 0x722d1c1e27d]   [kworker/2:0H]: bcl_read_lbat: LVLbat:5 val:0
```

### ipc -s
Save all ipc log.
```
crash> ipc -s
Save smp2p to xx/ipc_log/smp2p
Save rpm-glink to xx/ipc_log/rpm-glink

```
## reg
This command dumps the regulator info.

### reg -a
Display all regulator and consumer info.
```
crash> reg -a
regulator_dev:ffffff8006994800 regulator-dummy open_count:4 use_count:3 bypass_count:0 min_uV:0 max_uV:0 input_uV:0
   regulator:ffffff801e51c000 enable:0 load:0uA 5e94000,mdss_dsi0_ctrl-refgen
   regulator:ffffff801e51c0c0 enable:0 load:0uA 5e94400,mdss_dsi_phy0-gdsc
   regulator:ffffff801ad6cc00 enable:1 load:0uA soc:usb_nop_phy-vcc
   regulator:ffffff800a036780 enable:1 load:0uA regulator.21-SUPPLY
```
### reg -r
Display all regulator info.
```
crash> reg -r
regulator_dev    open use bypass regulator_desc   constraints      min_uV   max_uV   input_uV Name
ffffff8006994800 4    3   0      ffffffeee20a0020 ffffff80067f8d00 0        0        0        regulator-dummy
ffffff8007811800 17   0   0      ffffff8003999048 ffffff80033ebb00 0        0        0        gcc_camss_top_gdsc
ffffff8007813000 1    1   0      ffffff8003999448 ffffff80033eb700 0        0        0        gcc_usb20_prim_gdsc
ffffff8007812000 1    0   0      ffffff8003999c48 ffffff80033eb000 0        0        0        gcc_vcodec0_gdsc
ffffff8007810800 1    0   0      ffffff800a39a048 ffffff8007a39000 0        0        0        gcc_venus_gdsc
```
### reg -c 'name'
Display all consumer info of regulator.
```
crash> reg -c pm5100_l12
Consumers:
   regulator        load       enable Name
   ffffff802a899a80 0uA        0      5c54000,csiphy1-mipi-csi-vdd1
   ffffff802a883780 0uA        0      5c52000,csiphy0-mipi-csi-vdd1
   ffffff801e51ca80 0uA        0      5e94400,mdss_dsi_phy0-vdda-0p9 voltage:[904000uV~904000uV,]
   ffffff801ad6ce40 30000uA    1      1613000.hsphy-vdd voltage:[904000uV~904000uV,]
   ffffff8002204f00 0uA        1      regulator.59-SUPPLY
```

## icc
This command dumps the icc info.

### icc -a
Display all icc provider/node/request info.
```
crash> icc -a
icc_provider:ffffff880264a280 soc:interconnect@0
    icc_node:ffffff8808e67380 avg_bw:0 peak_bw:0 [qup0_core_master]
    icc_node:ffffff8808e67480 avg_bw:1 peak_bw:1 [qup1_core_master]
       icc_req:ffffff885db4b290 avg_bw:1 peak_bw:1 [a94000.i2c]
       icc_req:ffffff8862678710 avg_bw:1 peak_bw:1 [a8c000.i2c]
```
### icc -p
Display all icc provider.
```
crash> icc -p
icc_provider     users Name
ffffff880264a280 18    soc:interconnect@0
ffffff8807352a80 32    1600000.interconnect
ffffff880879ba80 34    1500000.interconnect
ffffff8808798c80 36    1680000.interconnect
ffffff8808799480 6     16c0000.interconnect
```
### icc -n 'provider'
Display all icc node of specified provider by provider name.
```
crash> icc -n 1600000.interconnect
icc_node         id    avg_bw     peak_bw    qcom_icc_node    Name
ffffff8808703980 39    115        14000      ffffffecd52b0c08 qsm_cfg
ffffff882e640500 513   0          0          ffffffecd52b0dc8 qhs_ahb2phy0
ffffff882e640880 514   0          0          ffffffecd52b0f88 qhs_ahb2phy1
ffffff882e640d00 516   0          0          ffffffecd52b1148 qhs_camera_cfg
ffffff8807b34100 517   0          0          ffffffecd52b1308 qhs_clk_ctl
```
### icc -r 'node'
Display all icc request of specified node by node name.
```
crash> icc -r qsm_cfg
icc_req          enabled tag  avg_bw     peak_bw    Name
ffffff8865f38cd0 true    0    0          0          a600000.ssusb
ffffff8857aaa4d0 false   0    1          1          1a88000.i2c
ffffff8857ad0cd0 false   0    1          1          884000.i2c
ffffff882abecad0 false   0    1          1          a94000.i2c
ffffff8857c782d0 false   0    1          1          880000.i3c-master
```

## ccf
This command dumps the clock info.

### ccf -c
Display all clock provider info.
```
crash> ccf -c
clk_provider: clock-controller
   clk_core               rate   req_rate   new_rate rpm   boot  en    prep  clk_hw           Name
   ffffff8017a8d600  1010.0MHZ   310.0MHZ  1010.0MHZ 0     1     1     1     ffffffd3034531a8 gpu_cc_pll0
   ffffff8017a8dc00     0.0MHZ     0.0MHZ     0.0MHZ 0     0     0     0     ffffffd3034533f0 gpu_cc_crc_ahb_clk
   ffffff8017a8dd00     0.0MHZ     0.0MHZ     0.0MHZ 0     0     0     0     ffffffd303453488 gpu_cc_cx_apb_clk
   ffffff8017a8d900  1010.0MHZ    19.2MHZ  1010.0MHZ 0     0     0     0     ffffffd303453520 gpu_cc_cx_gfx3d_clk
```
### ccf -t
Display all clock provider and clock consumers by tree.
```
crash> ccf -t
clk_provider:clock-controller
   clk_core:ffffff8017a8d600  gpu_cc_pll0                                   1010MHZ
   clk_core:ffffff8017a8d900  gpu_cc_cx_gfx3d_clk                           1010MHZ
   clk_core:ffffff8017a8d500  gpu_cc_cx_gfx3d_slv_clk                       1010MHZ
   clk_core:ffffff8017a8df00  gpu_cc_cx_gmu_clk                             19.2MHZ
           clk:ffffff801625a000  --> device:5900000.qcom,kgsl-3d0
```
### ccf -e
Display all enable clock info.
```
crash> ccf -e
=============================================
  Enable Clocks from of_clk_providers list
=============================================
clk_core         Name                                          Rate
ffffff801804ab00 dsi0_phy_pll_out_byteclk                      24.6847MHZ
ffffff801804aa00 dsi0_phy_pll_out_dsiclk                       8.22824MHZ
ffffff8017a8d600 gpu_cc_pll0                                   1010MHZ
```
### ccf -d
Display all disable clock info.
```
crash> ccf -d
=============================================
  Disabled Clocks from of_clk_providers list
=============================================
clk_core         Name                                          Rate
ffffff8017a8d500 gpu_cc_cx_gfx3d_slv_clk                       1010MHZ
ffffff800a68ed00 gpll4                                         806.4MHZ
ffffff800a68ef00 gpll7                                         808MHZ
```
### ccf -p
Display all prepare clock info.
```
crash> ccf -p
=============================================
  Prepare Clocks from of_clk_providers list
=============================================
clk_core         Name                                          Rate
ffffff801804ab00 dsi0_phy_pll_out_byteclk                      24.6847MHZ
ffffff801804aa00 dsi0_phy_pll_out_dsiclk                       8.22824MHZ
ffffff8017a8d600 gpu_cc_pll0                                   1010MHZ
```
## pstore
This command dumps the pstore log info.

### pstore -p
Display pmsg log.
```
crash> pstore -p
25-04-20 19:03:41.000758 1719  2299  1000   I SDM ClstcAlgorithmAdapter::QueryLibraryRequest():711 Get library config 0, rc 0
25-04-20 19:03:41.000766 1719  2299  1000   I SDM ClstcAlgorithmAdapter::QueryLibraryRequest():711 Get library config 0, rc 0
25-04-20 19:03:41.000775 1719  2299  1000   I SDM ClstcAlgorithmAdapter::QueryLibraryRequest():711 Get library config 0, rc 0
25-04-20 19:03:41.000775 7290  7911  1000   I lcomm.qti.axiom Waiting for a blocking GC Alloc
25-04-20 19:03:41.000782 1719  2299  1000   I SDM ClstcAlgorithmAdapter::QueryLibraryRequest():711 Get library config 0, rc 0
25-04-20 19:03:41.000791 1719  2299  1000   I SDM ClstcAlgorithmAdapter::QueryLibraryRequest():711 Get library config 0, rc 0
```

## sfg
This command dumps surfaceflinger info.

### sfg -a
Dump gralloc4 info
```
crash> sfg -a
GraphicBufferAllocator buffers:
      Handle |             Dmabuf |         Size |     W (Stride) x H | Layers |       Format |        Usage | Requestor
0x7486014340 | 0xffffff804f37e400 |   908.00 KiB |  454 ( 512) x  454 |      1 |    RGBA_8888 |       0x1b00 | FramebufferSurface
Total allocated by GraphicBufferAllocator (estimate): 1350.00 KB

Imported gralloc4 buffers:
Name                                    W/H                      Dmabuf                Format      Usage
FramebufferSurface                      454(512) x 454(464)      0xffffff804ea87000    RGBA_8888   0x1b00
```

### sfg -b
Dump gralloc5 info
```
crash> sfg -b
GraphicBufferAllocator buffers:
      Handle |             Dmabuf |         Size |     W (Stride) x H | Layers |       Format |        Usage | Requestor
0x7486014340 | 0xffffff804f37e400 |   908.00 KiB |  454 ( 512) x  454 |      1 |    RGBA_8888 |       0x1b00 | FramebufferSurface
Total allocated by GraphicBufferAllocator (estimate): 1350.00 KB

Imported gralloc5 buffers:
Name                                    W/H                      Dmabuf                Format      Usage
FramebufferSurface                      454(512) x 454(464)      0xffffff804ea87000    RGBA_8888   0x1b00
```

## env
This command dumps system info.

### env -c
View kernel command line
```
crash> env -c
init                           : /init
kernel.panic_on_rcu_stall      : 1
ftrace_dump_on_oops            :
ro                             :
kpti                           : off
video                          : vfb:640x400,bpp=32,memsize=3072000
```

### env -s
View soc info
```
crash> env -s
Chip                : xxx
Machine             : xxx
Soc ID              : 486
Serial Number       : 4080498789
HARDWARE            : xxx
Product ID          : 1077
PART_GPU            : Disable
```

## qlog
This command dumps boot log info.

### qlog -p
View pmic log
```
crash> qlog -p
Fundamental Reset: PON_PBL_STATUS=XVDD
, DVDD
; S3_RESET_REASON=None
FAULT_REASON1=None
; FAULT_REASON2=RESTART_PON
FAULT_REASON3=None
XVLO Check Complete
Trim Complete: 402 bytes written
KPDPWR_N
Begin PON Sequence
Waiting on PS_HOLD
Warm Reset Count: 0
PON Successful
```

### qlog -b
View boot log
```
crash> qlog -b
<6>[    0.000000][    T0] Booting Linux on physical CPU 0x0000000000 [0x412fd050]
<5>[    0.000000][    T0] Linux version 6.12.23-android16-5-maybe-dirty-debug (kleaf@build-host) (Android (12833971, +pgo, +bolt, +lto, +mlgo, based on r536225) clang version 19.0.1 (https://android.googlesource.com/toolchain/llvm-project b3a530ec6537146650e42be89f1089e9a3588460), LLD 19.0.1) #1 SMP PREEMPT Thu Jan  1 00:00:00 UTC 1970
<6>[    0.000000][    T0] KASLR enabled
<5>[    0.000000][    T0] random: crng init done
<6>[    0.000000][    T0] Machine model: Qualcomm Technologies, Inc. Vienna WDP
<4>[    0.000000][    T0] **********************************************************
<4>[    0.000000][    T0] **   NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE   **
<4>[    0.000000][    T0] **                                                      **
<4>[    0.000000][    T0] ** This system shows unhashed kernel memory addresses   **
<4>[    0.000000][    T0] ** via the console, logs, and other interfaces. This    **
<4>[    0.000000][    T0] ** might reduce the security of your system.            **
<4>[    0.000000][    T0] **                                                      **
<4>[    0.000000][    T0] ** If you see this message and you are not debugging    **
<4>[    0.000000][    T0] ** the kernel, report this immediately to your system   **
<4>[    0.000000][    T0] ** administrator!                                       **
<4>[    0.000000][    T0] **                                                      **
<4>[    0.000000][    T0] **   NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE   **
<4>[    0.000000][    T0] **********************************************************
<6>[    0.000000][    T0] panic_on_taint: bitmask=0x20 nousertaint_mode=disabled
```

### qlog -x
View xbl log
```
crash> qlog -x
Format: Log Type - Time(microsec) - Message - Optional Info
Log Type: B - Since Boot(Power On Reset),  D - Delta,  S - Statistic
```

## sched
This command dumps task sched info.

### sched -c 'cpu'
View the task sched info of the specified cpu.
```
crash> sched -c 0
Name                 pid     cpu Exec_Started    Last_Queued     Total_wait_time times_exec Prio  State Last_enqueued_ts Last_sleep_ts   Last_runtime
swapper/0            0       0   0               0               0               0          120   RU    0               0               0
swapper/0            0       0   0               0               0               0          120   RU    0               0               0
kworker/R-kvfre      4       0   0.0187329       0               1.3646e-05      2          100   ID    0               0               0
kworker/R-sync_      6       0   0.01976         0               6.8906e-05      2          100   ID    0               0               0
kworker/R-slub_      7       0   0.0202512       0               6.6823e-05      2          100   ID    0               0               0
```

## systemd
This command dumps journal log.

### systemd -l
List the journal log.
```
crash> systemd -l
╔══════════════════════════════════════════════════════════════════════════════╗
║                            SYSTEMD JOURNAL FILES                             ║
╚══════════════════════════════════════════════════════════════════════════════╝
Process Memory (VMA) - 2 files:
 [ 1] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000c340d-000000103eef2e0f.journal   8.00MB     SYSTEM
 [ 2] system.journal                                       8.00MB     SYSTEM

Page Cache (Inodes) - 12 files:
 [ 1] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000a892c-0000000e05db5bbd.journal      8MB  1382p  67%   SYSTEM
 [ 2] system.journal                                8MB  2048p 100%   SYSTEM
 [ 3] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000c340d-000000103eef2e0f.journal      8MB  1382p  67%   SYSTEM
 [ 4] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b8a19-0000000f5a5e7b48.journal      8MB  1382p  67%   SYSTEM
 [ 5] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000bdf13-0000000fccca8480.journal      8MB  1382p  67%   SYSTEM
 [ 6] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000ade28-0000000e77a80a2d.journal      8MB  1382p  67%   SYSTEM
 [ 7] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000c0990-0000001005f39a2b.journal      8MB  1382p  67%   SYSTEM
 [ 8] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b08a6-0000000eb0889fb1.journal      8MB  1381p  67%   SYSTEM
 [ 9] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b32da-0000000ee9031143.journal      8MB  1484p  72%   SYSTEM
 [10] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000bb496-0000000f939ee27e.journal      8MB  1382p  67%   SYSTEM
 [11] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000ab3aa-0000000e3ece08da.journal      8MB  1382p  67%   SYSTEM
 [12] system@bf00ed31f63c44bd8311c1e4185809e9-00000000000b5f9c-0000000f215ccc3c.journal   5.40MB  1382p 100%   SYSTEM

Summary Statistics:
   Total Journal Files: 14
     ├─ In Process Memory (VMA): 2          (Total: 16.01MB)
     └─ In Page Cache (Inode):   12         (Total: 93.40MB)

   Page Cache Statistics:
    ├─ Total Cached Pages: 17351
    └─ Cache Memory Usage: 67.78MB

Use 'systemd -d' to dump journal log
Use 'systemd -s' to show journal log contents

```
### systemd -d
Dump the journal log.
```
crash> systemd -d
Save system@bf00ed31f63c44bd8311c1e4185809e9-00000000000a892c-0000000e05db5bbd.journal to xxx/cache//system@bf00ed31f63c44bd8311c1e4185809e9-00000000000a892c-0000000e05db5bbd.journal
  - File size: 8388608 bytes
  - Pages processed: 1382
  - Pages written: 1382
  - Pages excluded: 0
  - Success rate: 100.0%

Journal dump summary:
  - Page cache files dumped: 12
  - VMA files dumped: 2
  - Total files dumped: 14
```

### systemd -s
View the journal log.
```
crash> systemd -s
Journal Logs (137135 entries):
================================================================================
Jan 02 00:43:47 pineapple kernel: [drm:sde_encoder_wait_for_event:6374] [sde error]enc66 intf_type:16, event:0 i:0, failed:-110
Jan 02 00:43:47 pineapple systemd-journal[320]: Data hash table of /run/log/journal/0306e3733ef9480a8ef2e76eaa2834f9/system.journal has a fill level at 75.0 (10923 of 14563 items, 8388608 file size, 767 bytes per hash table item), suggesting rotation.
Jan 02 00:43:47 pineapple systemd-journal[320]: /run/log/journal/0306e3733ef9480a8ef2e76eaa2834f9/system.journal: Journal header limits reached or header out-of-date, rotating.
Jan 02 00:43:47 pineapple kernel: [drm:sde_kms_wait_for_commit_done:1720] [sde error]crtc:209, enc:66, cwb_d:0, wait for commit done failed ret:-110
Jan 02 00:43:48 pineapple kernel: [drm:_sde_encoder_phys_cmd_handle_wr_ptr_timeout:2141] [sde error]enc66 intf1 wr_ptr_irq wait failed, switch_te:0
Jan 02 00:43:48 pineapple kernel: [drm:sde_connector_esd_status:2863] [sde error]ESD recovery already pending
```

## t32
This command generate launch_t32.bat script.

### t32 -s <the dump file path on Windows> -c <cpu_type>
generate the launch_t32.bat script
```
crash> t32 -s <path> -c <cpu_type>
Saved launch_t32.bat to path/t32/launch_t32.bat
```

## iommu
This command displays IOMMU (Input-Output Memory Management Unit) information.

### iommu -d
List all ARM SMMU devices with detailed hardware configuration.
```
crash> iommu -l
arm_smmu_device:0xffffff880a234000 [15000000.iommu]
    base                 : 0x15000000
    ioaddr               : 0x15000000
    numpage              : 256
    pgshift              : 12
    features             : 0x1234
    options              : 0x5678
    version              : ARM_SMMU_V2
    model                : QCOM_SMMUV2
    num_context_banks    : 64
    num_s2_context_banks : 32
    va_size              : 48
    ipa_size             : 40
    pa_size              : 48
    Context Banks:
       CB[05]:ttbr[0]=0x... ttbr[1]=0x... tcr[0]=0x... tcr[1]=0x... mair[0]=0x... mair[1]=0x... sctlr=0x...
             arm_smmu_cfg         : 0xffffff8808a5c100
               cbndx              : 5
               vmid               : 200
               procid             : 0
               cbar               : CBAR_TYPE_S2_TRANS
               fmt                : ARM_SMMU_CTX_FMT_AARCH64
```

### iommu -g
Display IOMMU groups with their associated devices and domains.
```
crash> iommu -g
arm_smmu_device:0xffffff880a234000[15000000.iommu]
    [003]iommu_group:0xffffff8808a5b000
         iommu_domain:0xffffff8808a5c000(10) IOMMU_DOMAIN_DMA [0x0000000000000000 ~ 0xffffffffffffffff]
         arm_smmu_domain      : 0xffffff8808a5bf00
           cbndx              : 5
           vmid               : 200
           procid             : 0
           cbar               : CBAR_TYPE_S2_TRANS
           fmt                : ARM_SMMU_CTX_FMT_AARCH64
           stage              : ARM_SMMU_DOMAIN_S2
           secure_vmid        : 0
           skip_tlb           : false
         SID:0x100    device:0xffffff8808a5d000[device_name]
```
### iommu -c
Display IOMMU client information.
```
crash> iommu -c
Total SMMU clients found: 16
1a80000.ethernet
    domain       : 0xffffff8af70a3608
    levels       : 3
    ttbr0        : 0xffffff8aff290000
    ttbr1        : 0
```
### iommu -p <client_name>
Display IOMMU client information.
```
crash> iommu -p 1a80000.ethernet
Client: 1a80000.ethernet
TTBR0: 0xffffff8b01d92000
Levels: 3
[VA Start -- VA End  ] [Size      ] [PA Start   -- PA End  ] [Attributes][Page Table Entry Size] [Memory Type] [Shareability] [Non-Executable]
0x0000000000000000--0x0000007f7fffffff [0x7f80000000] [UNMAPPED]
0x0000007f80000000--0x0000007fa55fffff [0x25600000] [UNMAPPED]
0x0000007fa5600000--0x0000007fa56fffff [0x100000] [UNMAPPED]
0x0000007fa5700000--0x0000007fa5700fff [0x1000] A:0x0000000d48c00000--0x0000000d48c00fff [0x1000] [R/W][4K] [Cached] [Inner-Shareable] [False]
0x0000007fa5701000--0x0000007fa5701fff [0x1000] A:0x0000000d48c01000--0x0000000d48c01fff [0x1000] [R/W][4K] [Cached] [Inner-Shareable] [False]
0x0000007fa5702000--0x0000007fa5702fff [0x1000] A:0x0000000d48c02000--0x0000000d48c02fff [0x1000] [R/W][4K] [Cached] [Inner-Shareable] [False]
```

## ftrace
This command dumps ftrace info.

### ftrace -l
List all trace array
```
crash> ftrace -l
usb_xhci
usb
```

### ftrace -a
Display all ftrace log
```
crash> ftrace -a
global     <idle>-0             [2] d.h1. 3471.171072: irq_handler_entry irq=3 name=IPI
global     <idle>-0             [3] d.h1. 3471.170575: irq_handler_entry irq=3 name=IPI
```

### ftrace -c <cpu>
Display ftrace log of specified trace array by cpu
```
crash> ftrace -c 1
global     sh-24270             [1] ..s1. 3471.158992: softirq_exit vec=9 [action=RCU]
global     sh-24270             [1] ..s1. 3471.158986: softirq_entry vec=9 [action=RCU]
```

### ftrace -s
Display ftrace log of specified trace array by name
```
crash> ftrace -s binder
binder     binder:943_4-1443    [0] ...1. 227.397729: binder_update_page_range proc=1067 allocate=1 offset=0 size=0
```

### ftrace -f
Display all trace event info
```
crash> ftrace -f
[25] ipi_raise: 'target_mask=%s (%s)', __get_bitmask(target_cpus), REC->reason
    format: 'target_mask=%s (%s)'
    args  :
        __get_bitmask(target_cpus)
        reason
    trace_event_raw_ipi_raise {
        __data_loc unsigned long[] target_cpus,offset:8, size:4
        const char * reason,     offset:16, size:8
    }
```

### ftrace -d
Dump all trace to file
```
crash> ftrace -d
Save to /path/ftrace.data
```

### ftrace -S
Display all trace via trace-cmd
```
crash> ftrace -S
memory:            <...>-67    [001]    43.835156: tlbi_end:             group=5800000.qcom,ipa:ipa_smmu_ap
memory:            <...>-67    [001]    43.835160: tlbi_start:           group=5800000.qcom,ipa:ipa_smmu_ap
```

## lockdep
Analyze Linux kernel lock dependency (lockdep) information

### lockdep -c
Display summary table of all tasks holding locks
```
crash> lockdep -c
PID    COMM            LOCKS TASK_STRUCT
------ -------------- ------ ----------------
1      systemd             1 0xffffff802726ca40
96     kworker/u33:0       7 0xffffff8027b94a40
492    kworker/u32:9       2 0xffffff8054ab2540
887    crtc_commit:19     48 0xffffff802579a540
1513   Binder_2            3 0xffffff88234e8040
1553   to transport        1 0xffffff803f37ca40
------ -------------- ------ ----------------
Total: 6 tasks holding locks out of 580 processed
```

### lockdep -d <task_addr>
Display detailed lock information for specific task
```
crash> lockdep -d 0xffffff802726ca40
1 locks held by systemd (pid=1, task_struct=0xffffff802726ca40):
#0: 0xffffffe25436dd30 (tty_mutex) at: tty_open
    ├─ Lock Mode        : EXCLUSIVE (write lock, mutual exclusion)
    ├─ Acquire Method   : BLOCKING (will wait until lock is available)
    ├─ IRQ State        : hardirqs_enabled, irq_context=0 (process context)
    ├─ Lock Validation  : check=disabled
    ├─ Reference Count  : references=0 (lockdep usage tracking), pin_count=0 (active holders)
    ├─ Timing           : holdtime=17.049303s, no race (fast path)
    ├─ Lock Status      : OK ACQUIRED
    └─ Debug            : class_idx=0x1b6, acquire_ip=0xffffffe2523b03f4, chain_key=0xffffffffffffffff (lockdep hash for deadlock detection)
```
