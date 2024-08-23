path='test2_results/flat_indexes'

cpupower frequency-set -d 4.5GHz
cpupower frequency-set -u 4.5GHz
for dataset in cora_fix flights adult restaurants; do
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for i in {0..4}; do
        echo "Run $i"
        echo 3 > /proc/sys/vm/drop_caches
        ./build/target/Desbordante_run $dataset.tsv $'\t' 0 > $path/$dataset/log$i.txt
    done
done

for dataset in notebook; do
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for i in {0..4}; do
        echo "Run $i"
        echo 3 > /proc/sys/vm/drop_caches
        ./build/target/Desbordante_run $dataset.csv ',' 1 > $path/$dataset/log$i.txt
    done
done

for dataset in cddb; do
    echo "Started $dataset"
    mkdir -p $path/$dataset
    for i in {0..4}; do
        echo "Run $i"
        echo 3 > /proc/sys/vm/drop_caches
        ./build/target/Desbordante_run $dataset.tsv $'\t' 1 > $path/$dataset/log$i.txt
    done
done
cpupower frequency-set -d 0.8GHz
cpupower frequency-set -u 4.5GHz
