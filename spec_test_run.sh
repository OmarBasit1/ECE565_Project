benchmarks=(
    perlbench_s 
    gcc_s 
    bwaves_s 
    mcf_s 
    cactuBSSN_s 
    lbm_s 
    omnetpp_s 
    wrf_s 
    xalancbmk_s 
    x264_s
    cam4_s 
    pop2_s 
    deepsjeng_s 
    imagick_s 
    leela_s 
    nab_s 
    exchange2_s 
    fotonik3d_s 
    roms_s 
    xz_s
    specrand_fs 
    specrand_is
)

for bench in "${benchmarks[@]}"; do
    build/X86/gem5.opt configs/deprecated/example/se.py --num-cpus=1 --cpu-type=X86O3CPU --l1d_size=32kB --l1d_assoc=8  --l1i_size=32kB --l1i_assoc=8 --caches --l2cache --l2_size=256kB --l2_assoc=8 --mem-size=8GB --maxinsts=100000000 --bench=$bench
    sleep 5
    mkdir -p m5out_init_bench_100M
    mv m5out m5out_init_bench_100M/m5out_$bench
done
