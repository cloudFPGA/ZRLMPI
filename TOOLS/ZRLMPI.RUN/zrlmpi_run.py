import os
import sys
import subprocess
import requests
import json
import time
import resource
import datetime

__openstack_pw__ = "fpga4all"
__openstack_user__ = "zrlngl"

__default_rank0_ip__ = "10.12.200.50"
__cf_manager_url__ = "9.4.136.104:8080"
__NON_FPGA_IDENTIFIER__ = "NON_FPGA"


def print_usage(argv0):
    print("USAGE: two options are possible: create new cluster or reuse an existing one.\n" +
            "(openstack credentials are saved.) \n" +
          # "    {0} new openstack_user openstack_pw role-open-stack-image-id number_of_nodes path/to/ZRLMPI/SW/rank0 \n".format(argv0) +
          # "    {0} reuse openstack_user openstack_pw open_stack_cluster_id path/to/ZRLMPI/SW/rank0 \n\n".format(argv0) )
            "    {0} new role-open-stack-image-id number_of_FPGA_nodes path/to/ZRLMPI/SW/rank0 \n".format(argv0) +
            "    {0} reuse open_stack_cluster_id path/to/ZRLMPI/SW/rank0 \n\n".format(argv0) )
    exit(1)

def errorReqExit(msg,code):
    print("Request "+msg+" failed with HTTP code "+str(code)+".\n")
    exit(1)


def create_new_cluster(number_of_FPGA_nodes, role_image_id):
    # build cluster_req structure
    print("Creating FPGA cluster...")
    cluster_req = []
    rank0node = {'image_id': __NON_FPGA_IDENTIFIER__,
                 'node_id': 0,
                  'node_ip': __default_rank0_ip__}
    cluster_req.append(rank0node)
    size = number_of_FPGA_nodes+1
    for i in range(1, size):
        fpgaNode = {
            'image_id': str(role_image_id),
            'node_id': i
        }
        cluster_req.append(fpgaNode)

    r1 = requests.post("http://"+__cf_manager_url__+"/clusters?username={0}&password={1}&prepare_mpi=1&mpi_size={2}".format(__openstack_user__,__openstack_pw__,size),
                       json=cluster_req)

    if r1.status_code != 200:
        # something went horrible wrong
        return errorReqExit("POST cluster", r1.status_code)

    cluster_data = json.loads(r1.text)
    print("Id of new cluster: {}".format(cluster_data['cluster_id']))
    return cluster_data


def get_cluster_data(cluster_id):
    print("Requesting cluster data...")
    r1 = requests.get("http://"+__cf_manager_url__+"/clusters/"+str(cluster_id)+"?username={0}&password={1}".format(__openstack_user__,__openstack_pw__))

    if r1.status_code != 200:
        # something went horrible wrong
        return errorReqExit("GET clusters", r1.status_code)

    cluster_data = json.loads(r1.text)
    return cluster_data


def restart_app(cluster_id):
    print("Restart all FPGAs...")
    r1 = requests.patch("http://"+__cf_manager_url__+"/clusters/"+str(cluster_id)+"/restart?username={0}&password={1}".format(__openstack_user__,__openstack_pw__))

    if r1.status_code != 200:
        # something went horrible wrong
        return errorReqExit("PATCH cluster restart", r1.status_code)

    return


def execute_mpi(path_to_rank0, number_of_nodes, cluster):
    # execute ping to ensuren correct ARP tables
    slot_list = [0]*number_of_nodes
    print("Ping all nodes, build ARP table...")
    for node in cluster['nodes']:
        if node['image_id'] == __NON_FPGA_IDENTIFIER__:
            continue
        # ping = subprocess.run(["ping", "-w3ms", "{0}".format(node['node_ip'])], stdout=subprocess.PIPE, cwd=os.getcwd())
        ping = subprocess.call(["ping","-c3" ,"-W2ms", "{0}".format(node['node_ip'])], stdout=subprocess.PIPE, cwd=os.getcwd())
        slot_list[node['node_id']] = str(node['slot_num'])

    # run mpi
    arg_list = [path_to_rank0, str(number_of_nodes)]
    arg_list.extend(slot_list[1:])

    print("Starting MPI subprocess at " + str(datetime.datetime.now()) + "...\n")
    mpi_start = time.time()
    #mpi_run = subprocess.run(arg_list, stdout=sys.stdout, cwd=os.getcwd())
    mpi_run = subprocess.call(arg_list, stdout=sys.stdout, cwd=os.getcwd())

    mpi_stop = time.time()

    info = resource.getrusage(resource.RUSAGE_CHILDREN)

    elapsed = (mpi_stop - mpi_start)
    print("\nMPI subprocess finished at "+str(datetime.datetime.now()) + ".\nMPI run elapsed time: {}s \n".format(elapsed))
    print("Time usage of child processes:\n\tUser mode \t{0:10.6f}s\n\tSystem mode \t{1:10.6f}s\n".format(info.ru_utime, info.ru_stime))

    return


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print_usage(sys.argv[0])

    #__openstack_user__ = sys.argv[2]
    #__openstack_pw__ = sys.argv[3]
    number_of_nodes = 0
    path_to_rank0 = ""
    cluster = None
    cluster_id = 0

    start = time.time()

    if sys.argv[1] == "new":
        number_of_FPGA_nodes = int(sys.argv[3])
        number_of_nodes = number_of_FPGA_nodes + 1
        cluster = create_new_cluster(number_of_FPGA_nodes, sys.argv[2])
        path_to_rank0 = sys.argv[4]
        cluster_id = cluster['cluster_id']
    elif sys.argv[1] == "reuse":
        cluster_id = sys.argv[2]
        path_to_rank0 = sys.argv[3]
        cluster = get_cluster_data(cluster_id)
        number_of_nodes = len(cluster['nodes'])
        restart_app(cluster_id)
    else:
        print_usage(sys.argv[0])

    ready = time.time()
    execute_mpi(path_to_rank0, number_of_nodes, cluster)

    done = time.time()
    elapsed1 = (ready - start)
    elapsed2 = (done - ready)
    elapsed3 = (done - start)
    print("Time for cluster setup: \t\t\t{0}s\nTime elapsed for mpi setup and execution: \t{1}s\nOverall elapsed time: \t\t\t\t{2}s \n".format(elapsed1,elapsed2, elapsed3))

