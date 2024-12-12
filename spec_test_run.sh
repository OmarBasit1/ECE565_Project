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
    build/X86/gem5.fast --outdir=/home/obasit/ece565/ECE565_Project/m5out_intel_baseline_700M/$bench configs/deprecated/example/se.py --num-cpus=1 --cpu-type=X86O3CPU --l1d_size=32kB --l1d_assoc=8  --l1i_size=32kB --l1i_assoc=8 --caches --l2cache --l2_size=256kB --l2_assoc=8 --mem-size=8GB --maxinsts=700000000 --bench=$bench &
done

# for bench in "${benchmarks[@]}"; do
#     build/X86/gem5.fast configs/deprecated/example/se.py --num-cpus=1 --cpu-type=X86O3CPU --l1d_size=32kB --l1d_assoc=8  --l1i_size=32kB --l1i_assoc=8 --caches --l2cache --l2_size=256kB --l2_assoc=8 --mem-size=8GB --maxinsts=70000 --bench=$bench --num-ROB-entries=2048 --num-phys-int-regs=190 --num-phys-fp-regs=168
#     sleep 5
#     mkdir -p tttm5out_intel_baseline_2048_ROB_700M
#     mv m5out tttm5out_intel_baseline_2048_ROB_700M/m5out_$bench
# done
