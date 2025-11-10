#!/bin/bash

# Usage: ./native_parallel.sh

./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 1 %s --num_pod=8" 2>&1 | tee native_lockstep_1.out 
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 4 %s --num_pod=8" 2>&1 | tee native_lockstep_4.out
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 8 %s --num_pod=8" 2>&1 | tee native_lockstep_8.out
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 16 %s --num_pod=8" 2>&1 | tee native_lockstep_16.out
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 32 %s --num_pod=8" 2>&1 | tee native_lockstep_32.out

./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 1 %s --num_pod=8" 2>&1 | tee native_nullmsg_1.out 
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 4 %s --num_pod=8" 2>&1 | tee native_nullmsg_4.out
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 8 %s --num_pod=8" 2>&1 | tee native_nullmsg_8.out
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 16 %s --num_pod=8" 2>&1 | tee native_nullmsg_16.out
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 32 %s --num_pod=8 --nullmsg" 2>&1 | tee native_nullmsg_32.out

./fat_tree_run.sh dc_fat_sim 1 --internal_traffic=true | tee splitsim_1.out
./fat_tree_run.sh dc_fat_sim 4 --internal_traffic=true | tee splitsim_4.out
./fat_tree_run.sh dc_fat_sim 8 --internal_traffic=true | tee splitsim_8.out
./fat_tree_run.sh dc_fat_sim 16 --internal_traffic=true | tee splitsim_16.out
./fat_tree_run.sh dc_fat_sim 32 --internal_traffic=true | tee splitsim_32.out

