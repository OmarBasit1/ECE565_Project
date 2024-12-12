# Copyright (c) 2012-2013 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Simple test script
#
# "m5 test.py"

import argparse
import os
import sys

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.params import NULL
from m5.util import (
    addToPath,
    fatal,
    warn,
)

from gem5.isas import ISA

addToPath("../../")

from common import (
    CacheConfig,
    CpuConfig,
    MemConfig,
    ObjectList,
    Options,
    Simulation,
)
from common.Caches import *
from common.cpu2000 import *
from common.FileSystemConfig import config_filesystem
from ruby import Ruby


def get_processes(args):
    """Interprets provided args and returns a list of processes"""

    multiprocesses = []
    inputs = []
    outputs = []
    errouts = []
    pargs = []

    workloads = args.cmd.split(";")
    if args.input != "":
        inputs = args.input.split(";")
    if args.output != "":
        outputs = args.output.split(";")
    if args.errout != "":
        errouts = args.errout.split(";")
    if args.options != "":
        pargs = args.options.split(";")

    idx = 0
    for wrkld in workloads:
        process = Process(pid=100 + idx)
        process.executable = wrkld
        process.cwd = os.getcwd()
        process.gid = os.getgid()

        if args.env:
            with open(args.env) as f:
                process.env = [line.rstrip() for line in f]

        if len(pargs) > idx:
            process.cmd = [wrkld] + pargs[idx].split()
        else:
            process.cmd = [wrkld]

        if len(inputs) > idx:
            process.input = inputs[idx]
        if len(outputs) > idx:
            process.output = outputs[idx]
        if len(errouts) > idx:
            process.errout = errouts[idx]

        multiprocesses.append(process)
        idx += 1

    if args.smt:
        cpu_type = ObjectList.cpu_list.get(args.cpu_type)
        assert ObjectList.is_o3_cpu(cpu_type), "SMT requires an O3CPU"
        return multiprocesses, idx
    else:
        return multiprocesses, 1


def my_get_processes(benchmark=None):
    bench_dir_17 = (
        "/home/obasit/ece565/link_to_export2_ECE_565/ECE565/precompiled/"
    )
    exe_dir_17 = (
        "/home/obasit/ece565/link_to_export2_ECE_565/ECE565/precompiled/"
    )
    output_dir = "/tmp/"
    exe_suffix = "_base.spectre_safebet-m64"

    # refrate process definitions unfinished (3 were started)
    refrate_run_dir = "run/run_base_refrate_spectre_safebet-m64.0000/"
    refspeed_run_dir = "run/run_base_refspeed_spectre_safebet-m64.0000/"

    # 603 bwaves_s
    bwaves_s = Process()
    bwaves_s_dir = "603.bwaves_s/"
    bwaves_s_run_dir = bwaves_s_dir + refspeed_run_dir
    bwaves_s.executable = (
        bench_dir_17 + bwaves_s_run_dir + "speed_bwaves" + exe_suffix
    )
    bwaves_s_data = "bwaves_1.in"
    bwaves_s.cmd = [bwaves_s.executable]
    bwaves_s.output = "bwaves_s.out"
    bwaves_s.input = bench_dir_17 + bwaves_s_run_dir + bwaves_s_data

    # 607 cactuBSSN_s
    cactuBSSN_s = Process()
    cactuBSSN_s_dir = "607.cactuBSSN_s/"
    cactuBSSN_s_run_dir = cactuBSSN_s_dir + refspeed_run_dir
    cactuBSSN_s.executable = (
        bench_dir_17 + cactuBSSN_s_run_dir + "cactuBSSN_s" + exe_suffix
    )
    cactuBSSN_s_data = "spec_ref.par"
    cactuBSSN_s.cmd = [cactuBSSN_s.executable] + [cactuBSSN_s_data]
    cactuBSSN_s.output = "cactuBSSN_s.out"
    cactuBSSN_s.cwd = bench_dir_17 + cactuBSSN_s_run_dir

    # 619 lbm_s
    lbm_s = Process()
    lbm_s_dir = "619.lbm_s/"
    lbm_s_run_dir = lbm_s_dir + refspeed_run_dir
    lbm_s.executable = bench_dir_17 + lbm_s_run_dir + "lbm_s" + exe_suffix
    lbm_s_data = "200_200_260_ldc.of"
    lbm_s.cmd = [lbm_s.executable] + [
        "2000",
        "reference.dat",
        "0",
        "0",
        lbm_s_data,
    ]
    lbm_s.output = "lbm_s.out"
    lbm_s.cwd = bench_dir_17 + lbm_s_run_dir

    # 621 wrf_s
    wrf_s = Process()
    wrf_s_dir = "621.wrf_s/"
    wrf_s_run_dir = wrf_s_dir + refspeed_run_dir
    wrf_s.executable = bench_dir_17 + wrf_s_run_dir + "wrf_s" + exe_suffix
    # no data
    wrf_s.cmd = [wrf_s.executable]
    wrf_s.output = "wrf_s.out"
    wrf_s.cwd = bench_dir_17 + wrf_s_run_dir

    # 627 cam4_s
    cam4_s = Process()
    cam4_s_dir = "627.cam4_s/"
    cam4_s_run_dir = cam4_s_dir + refspeed_run_dir
    cam4_s.executable = bench_dir_17 + cam4_s_run_dir + "cam4_s" + exe_suffix
    # no data
    cam4_s.cmd = [cam4_s.executable]
    cam4_s.output = "cam4_s.out"
    cam4_s.cwd = bench_dir_17 + cam4_s_run_dir

    # 628 pop2_s
    pop2_s = Process()
    pop2_s_dir = "628.pop2_s/"
    pop2_s_run_dir = pop2_s_dir + refspeed_run_dir
    pop2_s.executable = (
        bench_dir_17 + pop2_s_run_dir + "speed_pop2" + exe_suffix
    )
    # no data
    pop2_s.cmd = [pop2_s.executable]
    pop2_s.output = "pop2_s.out"
    pop2_s.cwd = bench_dir_17 + pop2_s_run_dir

    # 638 imagick_s
    imagick_s = Process()
    imagick_s_dir = "638.imagick_s/"
    imagick_s_run_dir = imagick_s_dir + refspeed_run_dir
    imagick_s.executable = (
        bench_dir_17 + imagick_s_run_dir + "imagick_s" + exe_suffix
    )
    imagick_s_data = "refspeed_input.tga"
    imagick_s.cmd = [imagick_s.executable] + [
        "-limit",
        "disk",
        "0",
        imagick_s_data,
        "-resize",
        "817%",
        "-rotate",
        "-2.76",
        "-shave",
        "540x375",
        "-alpha",
        "remove",
        "-auto-level",
        "-contrast-stretch",
        "1x1%",
        "-colorspace",
        "Lab",
        "-channel",
        "R",
        "-equalize",
        "+channel",
        "colorspace",
        "sRGB",
        "-define",
        "histogram:unique-colors=false",
        "-adaptive-blur",
        "0x5",
        "-despeckle",
        "-auto-gamma",
        "-adaptive-sharpen",
        "55",
        "-enhance",
        "-brightness-contrast",
        "10x10",
        "-resize",
        "30%",
        "refspeed_output.tga",
    ]
    imagick_s.output = "refspeed_convert.out"
    imagick_s.cwd = bench_dir_17 + imagick_s_run_dir

    # 644 nab_s
    nab_s = Process()
    nab_s_dir = "644.nab_s/"
    nab_s_run_dir = nab_s_dir + refspeed_run_dir
    nab_s.executable = bench_dir_17 + nab_s_run_dir + "nab_s" + exe_suffix
    nab_s_data = "3j1n"
    nab_s.cmd = [nab_s.executable] + [nab_s_data, "20140317", "220"]
    nab_s.output = "3j1n.out"
    nab_s.cwd = bench_dir_17 + nab_s_run_dir

    # 649 fotonik3d_s
    fotonik3d_s = Process()
    fotonik3d_s_dir = "649.fotonik3d_s/"
    fotonik3d_s_run_dir = fotonik3d_s_dir + refspeed_run_dir
    fotonik3d_s.executable = (
        bench_dir_17 + fotonik3d_s_run_dir + "fotonik3d_s" + exe_suffix
    )
    # no data
    fotonik3d_s.cmd = [fotonik3d_s.executable]
    fotonik3d_s.output = "fotonik3d_s.log"
    fotonik3d_s.cwd = bench_dir_17 + fotonik3d_s_run_dir

    # 654 rom_s
    roms_s = Process()
    roms_s_dir = "654.roms_s/"
    roms_s_run_dir = roms_s_dir + refspeed_run_dir
    roms_s.executable = bench_dir_17 + roms_s_run_dir + "sroms" + exe_suffix
    roms_s_data = "ocean_benchmark3.in.x"
    roms_s.cmd = [roms_s.executable]
    roms_s.output = "roms_s.out"
    roms_s.input = bench_dir_17 + roms_s_run_dir + roms_s_data
    roms_s.cwd = bench_dir_17 + roms_s_run_dir
    # 996 specrand_fs
    specrand_fs = Process()
    specrand_fs_dir = "996.specrand_fs/"
    specrand_fs_run_dir = specrand_fs_dir + refspeed_run_dir
    specrand_fs.executable = (
        bench_dir_17 + specrand_fs_run_dir + "specrand_fs" + exe_suffix
    )
    # no dataß
    specrand_fs.cmd = [specrand_fs.executable] + ["1255432124", "234923"]
    specrand_fs.output = "rand.234923.out"

    ## Integer Speed

    # 600 perlbench_s
    perlbench_s = Process()
    perlbench_s_dir = "600.perlbench_s/"
    perlbench_s_run_dir = perlbench_s_dir + refspeed_run_dir
    perlbench_s.executable = (
        bench_dir_17 + perlbench_s_run_dir + "perlbench_s" + exe_suffix
    )
    perlbench_s_data = "checkspam.pl"
    perlbench_s.cmd = [perlbench_s.executable] + [
        "-I./lib",
        perlbench_s_data,
        "2500",
        "5",
        "25",
        "11",
        "150",
        "1",
        "1",
        "1",
        "1",
    ]
    perlbench_s.output = "checkspam.2500.5.25.11.150.1.1.1.1.out"
    perlbench_s.cwd = bench_dir_17 + perlbench_s_run_dir

    # 602 gcc_s
    gcc_s = Process()
    gcc_s_dir = "602.gcc_s/"
    gcc_s_run_dir = gcc_s_dir + refspeed_run_dir
    gcc_s.executable = bench_dir_17 + gcc_s_run_dir + "sgcc" + exe_suffix
    gcc_s_data = "gcc-pp.c"
    gcc_s.cmd = [gcc_s.executable] + [
        gcc_s_data,
        "-O5",
        "-finline-limit=24000",
        "-fgcse",
        "-fgcse-las",
        "-fgcse-lm",
        "-fgcse-sm",
        "-o",
        "gcc-pp.opts-O5_-finline-limit_24000_-fgcse_-fgcse-las_-fgcse-lm_-fgcse-sm.s",
    ]
    gcc_s.output = "gcc-pp.opts-O5_-finline-limit_24000_-fgcse_-fgcse-las_-fgcse-lm_-fgcse-sm.out"
    gcc_s.cwd = bench_dir_17 + gcc_s_run_dir

    # 605 mcf_s
    mcf_s = Process()
    mcf_s_dir = "605.mcf_s/"
    mcf_s_run_dir = mcf_s_dir + refspeed_run_dir
    mcf_s.executable = bench_dir_17 + mcf_s_run_dir + "mcf_s" + exe_suffix
    mcf_s_data = "inp.in"
    mcf_s.cmd = [mcf_s.executable] + [mcf_s_data]
    mcf_s.output = "inp.out"
    mcf_s.cwd = bench_dir_17 + mcf_s_run_dir

    # 620 omnetpp_s
    omnetpp_s = Process()
    omnetpp_s_dir = "620.omnetpp_s/"
    omnetpp_s_run_dir = omnetpp_s_dir + refspeed_run_dir
    omnetpp_s.executable = (
        bench_dir_17 + omnetpp_s_run_dir + "omnetpp_s" + exe_suffix
    )
    # no data?
    omnetpp_s.cmd = [omnetpp_s.executable] + ["-c", "General", "-r", "0"]
    omnetpp_s.output = "omnetpp.General-0.out"
    omnetpp_s.cwd = bench_dir_17 + omnetpp_s_run_dir

    # 623 xalancbmk_s
    xalancbmk_s = Process()
    xalancbmk_s_dir = "623.xalancbmk_s/"
    xalancbmk_s_run_dir = xalancbmk_s_dir + refspeed_run_dir
    xalancbmk_s.executable = (
        bench_dir_17 + xalancbmk_s_run_dir + "xalancbmk_s" + exe_suffix
    )
    xalancbmk_s_data_1 = "t5.xml"
    xalancbmk_s_data_2 = "xalanc.xsl"
    xalancbmk_s.cmd = [xalancbmk_s.executable] + [
        "-v",
        xalancbmk_s_data_1,
        xalancbmk_s_data_2,
    ]
    xalancbmk_s.output = "ref-t5.out"
    xalancbmk_s.cwd = bench_dir_17 + xalancbmk_s_run_dir

    # 625 x264_s
    x264_s = Process()
    x264_s_dir = "625.x264_s/"
    x264_s_run_dir = x264_s_dir + refspeed_run_dir
    x264_s.executable = bench_dir_17 + x264_s_run_dir + "x264_s" + exe_suffix
    # no data
    x264_s.cmd = [x264_s.executable] + [
        "--pass",
        "1",
        "--stats",
        "x264_stats.log",
        "--bitrate",
        "1000",
        "--frames",
        "1000",
        "-o",
        "BuckBunny_New.264",
        "BuckBunny.yuv",
        "1280x720",
    ]
    x264_s.output = (
        "run_000-1000_x264_s_base.spectre_safebet-m64_x264_pass1.out"
    )
    x264_s.cwd = bench_dir_17 + x264_s_run_dir

    # 631 deepsjeng_s
    deepsjeng_s = Process()
    deepsjeng_s_dir = "631.deepsjeng_s/"
    deepsjeng_s_run_dir = deepsjeng_s_dir + refspeed_run_dir
    deepsjeng_s.executable = (
        bench_dir_17 + deepsjeng_s_run_dir + "deepsjeng_s" + exe_suffix
    )
    deepsjeng_s_data = "ref.txt"
    deepsjeng_s.cmd = [deepsjeng_s.executable] + [deepsjeng_s_data]
    deepsjeng_s.output = "ref.out"
    deepsjeng_s.cwd = bench_dir_17 + deepsjeng_s_run_dir

    # 641 leela_s
    leela_s = Process()
    leela_s_dir = "641.leela_s/"
    leela_s_run_dir = leela_s_dir + refspeed_run_dir
    leela_s.executable = (
        bench_dir_17 + leela_s_run_dir + "leela_s" + exe_suffix
    )
    leela_s_data = "ref.sgf"
    leela_s.cmd = [leela_s.executable] + [leela_s_data]
    leela_s.output = output_dir + "ref.out"
    leela_s.cwd = bench_dir_17 + leela_s_run_dir

    # 648 exchange2_s
    exchange2_s = Process()
    exchange2_s_dir = "648.exchange2_s/"
    exchange2_s_run_dir = exchange2_s_dir + refspeed_run_dir
    exchange2_s.executable = (
        bench_dir_17 + exchange2_s_run_dir + "exchange2_s" + exe_suffix
    )
    # no data
    exchange2_s.cmd = [exchange2_s.executable] + ["6"]
    exchange2_s.output = "exchange2.txt"
    exchange2_s.cwd = bench_dir_17 + exchange2_s_run_dir

    # 657 xz_s
    xz_s = Process()
    xz_s_dir = "657.xz_s/"
    xz_s_run_dir = xz_s_dir + refspeed_run_dir
    xz_s.executable = bench_dir_17 + xz_s_run_dir + "xz_s" + exe_suffix
    xz_s_data = "cld.tar.xz"
    # xz_s_data = 'cpu2006docs.tar.xz'
    # xz_s.cmd = [xz_s.executable] + [xz_s_data,'6643',
    #    '1036078272','1111795472','4']
    xz_s.cmd = [xz_s.executable] + [
        xz_s_data,
        "1400",
        "19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474",
        "536995164",
        "539938872",
        "8",
    ]
    xz_s.output = "cpu2006docs.tar-6643-4.out"
    xz_s.cwd = bench_dir_17 + xz_s_run_dir

    # 998 specrand_is
    specrand_is = Process()
    specrand_is_dir = "998.specrand_is/"
    specrand_is_run_dir = specrand_is_dir + refspeed_run_dir
    specrand_is.executable = (
        bench_dir_17 + specrand_is_run_dir + "specrand_is" + exe_suffix
    )
    # no data
    specrand_is.cmd = [specrand_is.executable] + ["1255432124", "234923"]
    specrand_is.output = "rand.234923.out"

    ## Floating Point Rate

    # unused at the moment

    # bwaves_r
    bwaves_r = Process()
    bwaves_r_dir = "503.bwaves_r/"
    bwaves_r_run_dir = bwaves_r_dir + refrate_run_dir
    bwaves_r.executable = (
        bench_dir_17 + bwaves_r_run_dir + "bwaves_r" + exe_suffix
    )
    bwaves_r_data = "bwaves_1.in"
    bwaves_r.cmd = [bwaves_r.executable] + [bwaves_r_data]
    bwaves_r.output = "bwaves.out"

    # cactuBSSN_r
    cactuBSSN_r = Process()
    cactuBSSN_r_dir = "507.cactuBSSN_r"
    cactuBSSN_r_run_dir = cactuBSSN_r_dir + refrate_run_dir
    cactuBSSN_r.executable = (
        bench_dir_17 + cactuBSSN_r_run_dir + "cactuBSSN_r" + exe_suffix
    )
    cactuBSSN_r_data = "spec_ref.par"
    cactuBSSN_r.cmd = [cactuBSSN_r.executable] + [cactuBSSN_r_data]
    cactuBSSN_r.output = "cactuBSSN.out"

    # lbm_r
    lbm_r = Process()
    lbm_r_dir = "519.lbm_r/"
    # lbm_run_dir = 'run/run_base_test_spectre_safebet-m64.0000/'
    lbm_r_run_dir = lbm_r_dir + refrate_run_dir
    lbm_r.executable = (
        bench_dir_17 + lbm_r_run_dir + "lbm_r_base.spectre_safebet-m64"
    )
    lbm_r_data = "100_100_130_cf_a.of"
    lbm_r.cmd = [lbm_r.executable] + [
        "20",
        "reference.dat",
        "0",
        "1",
        lbm_r_data,
    ]
    lbm_r.output = "lbm.out"

    benchtype = "-m64-gcc43-nn"

    if benchmark == "perlbench_s":
        process = perlbench_s
    elif benchmark == "gcc_s":
        process = gcc_s
    elif benchmark == "bwaves_s":
        process = bwaves_s
    elif benchmark == "mcf_s":
        process = mcf_s
    elif benchmark == "cactuBSSN_s":
        process = cactuBSSN_s
    elif benchmark == "deepsjeng_s":
        process = deepsjeng_s
    elif benchmark == "lbm_s":
        process = lbm_s
    elif benchmark == "omnetpp_s":
        process = omnetpp_s
    elif benchmark == "wrf_s":
        process = wrf_s
    elif benchmark == "xalancbmk_s":
        process = xalancbmk_s
    elif benchmark == "specrand_is":
        process = specrand_is
    elif benchmark == "specrand_fs":
        process = specrand_fs
    elif benchmark == "cam4_s":
        process = cam4_s
    elif benchmark == "pop2_s":
        process = pop2_s
    elif benchmark == "imagick_s":
        process = imagick_s
    elif benchmark == "nab_s":
        process = nab_s
    elif benchmark == "fotonik3d_s":
        process = fotonik3d_s
    elif benchmark == "roms_s":
        process = roms_s
    elif benchmark == "x264_s":
        process = x264_s
    elif benchmark == "leela_s":
        process = leela_s
    elif benchmark == "exchange2_s":
        process = exchange2_s
    elif benchmark == "xz_s":
        process = xz_s
    elif benchmark == "perlbench":
        process = perlbench
    elif benchmark == "bzip2":
        process = bzip2
    elif benchmark == "gcc":
        process = gcc
    elif benchmark == "bwaves":
        process = bwaves
    elif benchmark == "gamess":
        process = gamess
    elif benchmark == "mcf":
        process = mcf
    elif benchmark == "milc":
        process = milc
    elif benchmark == "zeusmp":
        process = zeusmp
    elif benchmark == "gromacs":
        process = gromacs
        shutil.copy(os.path.join(process.cwd, "gromacs.tpr"), os.getcwd())
    elif benchmark == "cactusADM":
        process = cactusADM
    elif benchmark == "leslie3d":
        process = leslie3d
    elif benchmark == "namd":
        process = namd
    elif benchmark == "gobmk":
        process = gobmk
    elif benchmark == "dealII":
        process = dealII
    elif benchmark == "soplex":
        process = soplex
    elif benchmark == "povray":
        process = povray
    elif benchmark == "calculix":
        process = calculix
    elif benchmark == "hmmer":
        process = hmmer
    elif benchmark == "sjeng":
        process = sjeng
    elif benchmark == "GemsFDTD":
        process = GemsFDTD
    elif benchmark == "libquantum":
        process = libquantum
    elif benchmark == "h264ref":
        process = h264ref
    elif benchmark == "tonto":
        process = tonto
    elif benchmark == "lbm":
        process = lbm
    elif benchmark == "omnetpp":
        process = omnetpp
    elif benchmark == "astar":
        process = astar
    elif benchmark == "wrf":
        process = wrf
    elif benchmark == "sphinx3":
        process = sphinx3
    elif benchmark == "xalancbmk":
        process = xalancbmk
    elif benchmark == "specrand_i":
        process = specrand_i
    elif benchmark == "specrand_f":
        process = specrand_f
    return process, 1


warn(
    "The se.py script is deprecated. It will be removed in future releases of "
    " gem5."
)

parser = argparse.ArgumentParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)

if "--ruby" in sys.argv:
    Ruby.define_options(parser)

args = parser.parse_args()

multiprocesses = []
numThreads = 1


multiprocesses, numThreads = my_get_processes(args.bench)


(CPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(args)
CPUClass.numThreads = numThreads

# Check -- do not allow SMT with multiple CPUs
if args.smt and args.num_cpus > 1:
    fatal("You cannot use SMT with multiple CPUs!")

np = args.num_cpus
mp0_path = multiprocesses[0].executable
system = System(
    cpu=[CPUClass(cpu_id=i) for i in range(np)],
    mem_mode=test_mem_mode,
    mem_ranges=[AddrRange(args.mem_size)],
    cache_line_size=args.cacheline_size,
)

if numThreads > 1:
    system.multi_thread = True

# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage=args.sys_voltage)

# Create a source clock for the system and set the clock period
system.clk_domain = SrcClockDomain(
    clock=args.sys_clock, voltage_domain=system.voltage_domain
)

# Create a CPU voltage domain
system.cpu_voltage_domain = VoltageDomain()

# Create a separate clock domain for the CPUs
system.cpu_clk_domain = SrcClockDomain(
    clock=args.cpu_clock, voltage_domain=system.cpu_voltage_domain
)

# If elastic tracing is enabled, then configure the cpu and attach the elastic
# trace probe
if args.elastic_trace_en:
    CpuConfig.config_etrace(CPUClass, system.cpu, args)

# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
for cpu in system.cpu:
    cpu.clk_domain = system.cpu_clk_domain
    cpu.set_ROB_entries(args.num_ROB_entries),
    cpu.set_IQ_entries(args.num_IQ_entries),
    cpu.set_numPhysIntRegs(args.num_phys_int_regs),
    cpu.set_numPhysFloatRegs(args.num_phys_fp_regs),

if ObjectList.is_kvm_cpu(CPUClass) or ObjectList.is_kvm_cpu(FutureClass):
    if buildEnv["USE_X86_ISA"]:
        system.kvm_vm = KvmVM()
        system.m5ops_base = max(0xFFFF0000, Addr(args.mem_size).getValue())
        for process in multiprocesses:
            process.useArchPT = True
            process.kvmInSE = True
    else:
        fatal("KvmCPU can only be used in SE mode with x86")

# Sanity check
if args.simpoint_profile:
    if not ObjectList.is_noncaching_cpu(CPUClass):
        fatal("SimPoint/BPProbe should be done with an atomic cpu")
    if np > 1:
        fatal("SimPoint generation not supported with more than one CPUs")

for i in range(np):
    if args.smt:
        system.cpu[i].workload = multiprocesses
    elif len(multiprocesses) == 1:
        system.cpu[i].workload = multiprocesses[0]
    else:
        system.cpu[i].workload = multiprocesses[i]

    if args.simpoint_profile:
        system.cpu[i].addSimPointProbe(args.simpoint_interval)

    if args.checker:
        system.cpu[i].addCheckerCpu()

    if args.bp_type:
        bpClass = ObjectList.bp_list.get(args.bp_type)
        system.cpu[i].branchPred = bpClass()

    if args.indirect_bp_type:
        indirectBPClass = ObjectList.indirect_bp_list.get(
            args.indirect_bp_type
        )
        system.cpu[i].branchPred.indirectBranchPred = indirectBPClass()

    system.cpu[i].createThreads()

if args.ruby:
    Ruby.create_system(args, False, system)
    assert args.num_cpus == len(system.ruby._cpu_ports)

    system.ruby.clk_domain = SrcClockDomain(
        clock=args.ruby_clock, voltage_domain=system.voltage_domain
    )
    for i in range(np):
        ruby_port = system.ruby._cpu_ports[i]

        # Create the interrupt controller and connect its ports to Ruby
        # Note that the interrupt controller is always present but only
        # in x86 does it have message ports that need to be connected
        system.cpu[i].createInterruptController()

        # Connect the cpu's cache ports to Ruby
        ruby_port.connectCpuPorts(system.cpu[i])
else:
    MemClass = Simulation.setMemClass(args)
    system.membus = SystemXBar()
    system.system_port = system.membus.cpu_side_ports
    CacheConfig.config_cache(args, system)
    MemConfig.config_mem(args, system)
    config_filesystem(system, args)

system.workload = SEWorkload.init_compatible(mp0_path)

if args.wait_gdb:
    system.workload.wait_for_remote_gdb = True

root = Root(full_system=False, system=system)
Simulation.run(args, root, system, FutureClass)
