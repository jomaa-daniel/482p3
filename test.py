import subprocess
import random
import pathlib
import time
import os
import threading
abs_path = str(pathlib.Path().absolute())


TIMEOUT = 20
ITERATION = 0.5

# run the pager
pager_args = [abs_path + "/pager", "-s", "10"]

test_cases = ["test_copy_on_write_last_owner", "test_copy_on_write_shared_swap", 
"test_double_child_sharing", "test_double_child_sharing", "test_evict_clean_dirty_S",
"test_evict_nowriteback_NS", "test_evict_shared_corrs_swap_S", "test_evict_shared_swap_S", "test_evict_writeback_NS",
  "test_fork_chain_no_work",  "test_reclaim_ppage", "test_reuse_ppage_NS",
 "test_simple", "test_simplified_fork"]
 
 #"test_max_swap_blocks_NS "test_fault_on_swap", NOT TESTED DUE TO SIZE

random.shuffle(test_cases)

for test in test_cases:
    test_name = "g++ -g -Wall -fno-builtin -o " + test + " " + test + ".4.cpp " + "libvm_app_core.o -ldl"
    subprocess.call(test_name.split(" "))

print("start")

for test in test_cases:
    print("Running Test: ", test)
    test_process = subprocess.Popen([abs_path + "/" + test], stdout=subprocess.PIPE)
    
    poll_test = test_process.poll()
    cur_time = 0
    # while poll_test is None and cur_time <= TIMEOUT:
    #     time.sleep(ITERATION)
    #     cur_time += ITERATION
    #     poll_test = test_process.poll()
    
    # if cur_time >= TIMEOUT:
    #     print("timed out")
    #     exit

    print(test_process.stdout.read().decode("utf-8"))
    
    print("------- test finished -------")


for test in test_cases:
    subprocess.call(["rm", test])