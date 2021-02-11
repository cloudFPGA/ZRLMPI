#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Dec 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        ZRLMPIrun script
#  *
#  *

import os
import sys
import subprocess
import requests
import json
import time
import resource
import datetime

__openstack_pw__ = "XX"
__openstack_user__ = "YY"
__openstack_project__ = "default"


__credentials_file_name__ = "user.json"

__openstack_user_template__ = {}
__openstack_user_template__['credentials'] = {}
__openstack_user_template__['credentials']['user'] = "your user name"
__openstack_user_template__['credentials']['pw'] = "your user password"
__openstack_user_template__['project'] = "default"

__cf_manager_url__ = "10.12.0.132:8080"
__NON_FPGA_IDENTIFIER__ = "NON_FPGA"


def print_usage(argv0):
    print("USAGE: two options are possible: create new cluster or reuse an existing one.\n" +
            "Openstack credentials should be stored in {}) \n".format(__credentials_file_name__) +
            "    {0} new <udp|tcp> host_address role-open-stack-image-id number_of_FPGA_nodes path/to/ZRLMPI/SW/binary SW_node_id\n".format(argv0) +
            "    {0} reuse <udp|tcp> host_address open_stack_cluster_id path/to/ZRLMPI/SW/binary \n\n".format(argv0) )
    exit(1)


def errorReqExit(msg, code):
    print("Request "+msg+" failed with HTTP code "+str(code)+".\n")
    exit(1)


def create_new_cluster(number_of_FPGA_nodes, role_image_id, host_address, sw_rank):
    # build cluster_req structure
    print("Creating FPGA cluster...")
    cluster_req = []
    rank0node = {'image_id': __NON_FPGA_IDENTIFIER__,
                 'node_id': int(sw_rank),
                  'node_ip': host_address}
    cluster_req.append(rank0node)
    size = number_of_FPGA_nodes+1
    for i in range(1, size):
        fpgaNode = {
            'image_id': str(role_image_id),
            'node_id': int(i)
        }
        cluster_req.append(fpgaNode)

    # r1 = requests.post("http://"+__cf_manager_url__+"/clusters?username={0}&password={1}&prepare_mpi=1&mpi_size={2}".format(__openstack_user__,__openstack_pw__,size),
    #                   json=cluster_req)
    r1 = requests.post("http://"+__cf_manager_url__+"/clusters?username={0}&password={1}&project_name={2}&dont_verify_memory=1".format(
                        __openstack_user__, __openstack_pw__, __openstack_project__),
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


def execute_mpi(path_to_sw_binary, protocol, host_address, number_of_nodes, cluster):
    # execute ping to ensuren correct ARP tables
    slot_ip_list = [0]*number_of_nodes
    print("Ping all nodes, build ARP table...")
    sw_node_id = 0
    for node in cluster['nodes']:
        if node['image_id'] == __NON_FPGA_IDENTIFIER__:
            sw_node_id = node['node_id']
            slot_ip_list[sw_node_id] = __NON_FPGA_IDENTIFIER__
            continue
#        subprocess.call(["/usr/bin/ping","-c3" ,"-W2ms", "{0}".format(node['node_ip'])], stdout=subprocess.PIPE, cwd=os.getcwd())
        subprocess.call(["/usr/bin/ping","-I{}".format(host_address) , "-c3", "{0}".format(node['node_ip'])], stdout=subprocess.PIPE, cwd=os.getcwd())
        # print("/usr/bin/ping","-I{}".format(str(host_address)) , "-c3", "{0}".format(node['node_ip']))
        slot_ip_list[node['node_id']] = str(node['node_ip'])

    # run mpi
    # Usage: ./zrlmpi_cpu_app <tcp|udp> <host-address> <cluster-size> <own-rank> <ip-rank-1> <ip-rank-2> <...>
    arg_list = [str(path_to_sw_binary), str(protocol), str(host_address), str(number_of_nodes), str(sw_node_id)]
    for e in slot_ip_list:
        if e != __NON_FPGA_IDENTIFIER__:
            arg_list.append(e)
    # print(arg_list)

    print("Starting MPI subprocess at " + str(datetime.datetime.now()) + "...\n")
    mpi_start = time.time()
    mpi_run = subprocess.call(arg_list, stdout=sys.stdout, cwd=os.getcwd())

    mpi_stop = time.time()

    info = resource.getrusage(resource.RUSAGE_CHILDREN)

    elapsed = (mpi_stop - mpi_start)
    print("\nMPI subprocess finished at "+str(datetime.datetime.now()) + ".\nMPI run elapsed time: {}s \n".format(elapsed))
    print("Time usage of child processes:\n\tUser mode \t{0:10.6f}s\n\tSystem mode \t{1:10.6f}s\n".format(info.ru_utime, info.ru_stime))

    return


def load_user_credentials(filedir):
    json_file = filedir + "/" + __credentials_file_name__
    global __openstack_user__
    global __openstack_pw__
    global __openstack_project__

    try:
        with open(json_file, 'r') as infile:
            data = json.load(infile)
        __openstack_user__ = data['credentials']['user']
        __openstack_pw__ = data['credentials']['pw']
        if 'project' in data:
            __openstack_project__ = data['project']
        return 0
    except Exception as e:
        print(e)
        print("Writing credentials template to {}\n".format(json_file))

    with open(json_file, 'w') as outfile:
        json.dump(__openstack_user_template__, outfile)
    return -1


if __name__ == '__main__':
    if len(sys.argv) < 6:
        print_usage(sys.argv[0])

    exec_dir = os.path.dirname(sys.argv[0])
    if load_user_credentials(exec_dir) == -1:
        print("Please save your OpenStack credentials in {}/{}".format(exec_dir,__credentials_file_name__))
        exit(1)

    number_of_nodes = 0
    path_to_sw_binary = ""
    cluster = None
    cluster_id = 0

    protocol = "X"
    if sys.argv[2] != "udp":
        print("Only UDP is supported for the moment.\n")
        exit(1)
        # TODO
    else:
        protocol = "udp"

    host_address = sys.argv[3]

    start = time.time()

    if sys.argv[1] == "new":
        number_of_FPGA_nodes = int(sys.argv[5])
        number_of_nodes = number_of_FPGA_nodes + 1
        sw_rank = sys.argv[7]
        cluster = create_new_cluster(number_of_FPGA_nodes, sys.argv[4], host_address, sw_rank)
        path_to_sw_binary = sys.argv[6]
        cluster_id = cluster['cluster_id']
    elif sys.argv[1] == "reuse":
        cluster_id = sys.argv[4]
        path_to_sw_binary = sys.argv[5]
        cluster = get_cluster_data(cluster_id)
        number_of_nodes = len(cluster['nodes'])
        restart_app(cluster_id)
    else:
        print_usage(sys.argv[0])

    ready = time.time()
    execute_mpi(path_to_sw_binary, protocol, str(host_address), number_of_nodes, cluster)

    done = time.time()
    elapsed1 = (ready - start)
    elapsed2 = (done - ready)
    elapsed3 = (done - start)
    print("Time for cluster setup: \t\t\t{0}s\nTime elapsed for mpi setup and execution: \t{1}s\nOverall elapsed time: \t\t\t\t{2}s \n".format(elapsed1, elapsed2, elapsed3))

