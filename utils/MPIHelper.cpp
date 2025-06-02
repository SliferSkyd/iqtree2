//
// Created by tung on 6/18/15.
//

#include "MPIHelper.h"
#include "timeutil.h"
#include <queue>

/**
 *  Initialize the single getInstance of MPIHelper
 */

MPIHelper& MPIHelper::getInstance() {
    static MPIHelper instance;
#ifndef _IQTREE_MPI
    instance.setProcessID(0);
    instance.setNumProcesses(1);
#endif
    return instance;
}

void MPIHelper::init(int argc, char *argv[]) {
#ifdef _IQTREE_MPI
    int n_tasks, task_id;
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        outError("MPI initialization failed!");
    }
    MPI_Comm_size(MPI_COMM_WORLD, &n_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &task_id);
    setNumProcesses(n_tasks);
    setProcessID(task_id);
    setNumTreeReceived(0);
    setNumTreeSent(0);
    setNumNNISearch(0);
#endif
}

void MPIHelper::finalize() {
#ifdef _IQTREE_MPI
    MPI_Finalize();
#endif
}

void MPIHelper::syncRandomSeed() {
#ifdef _IQTREE_MPI
    unsigned int rndSeed;
    if (MPIHelper::getInstance().isMaster()) {
        rndSeed = Params::getInstance().ran_seed;
    }
    // Broadcast random seed
    MPI_Bcast(&rndSeed, 1, MPI_INT, PROC_MASTER, MPI_COMM_WORLD);
    if (MPIHelper::getInstance().isWorker()) {
        //        Params::getInstance().ran_seed = rndSeed + task_id * 100000;
        Params::getInstance().ran_seed = rndSeed;
        //        printf("Process %d: random_seed = %d\n", task_id, Params::getInstance().ran_seed);
    }
#endif
}

int MPIHelper::countSameHost() {
#ifdef _IQTREE_MPI
    // detect if processes are in the same host
    char host_name[MPI_MAX_PROCESSOR_NAME];
    int resultlen;
    /*int pID =*/ (void) MPIHelper::getInstance().getProcessID();
    MPI_Get_processor_name(host_name, &resultlen);
    char *host_names;
    host_names = new char[MPI_MAX_PROCESSOR_NAME * MPIHelper::getInstance().getNumProcesses()];
    
    MPI_Allgather(host_name, resultlen+1, MPI_CHAR, host_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
               MPI_COMM_WORLD);
    
    int count = 0;
    for (int i = 0; i < MPIHelper::getInstance().getNumProcesses(); i++)
        if (strcmp(&host_names[i*MPI_MAX_PROCESSOR_NAME], host_name) == 0)
            count++;
    delete [] host_names;
    if (count>1)
        cout << "NOTE: " << count << " processes are running on the same host " << host_name << endl; 
    return count;
#else
    return 1;
#endif
}

bool MPIHelper::gotMessage() {
    // Check for incoming messages
    if (getNumProcesses() == 1)
        return false;
#ifdef _IQTREE_MPI
    int flag = 0;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    if (flag)
        return true;
    else
        return false;
#else
    return false;
#endif
}



#ifdef _IQTREE_MPI
void MPIHelper::sendString(string &str, int dest, int tag) {
    char *buf = (char*)str.c_str();
    MPI_Send(buf, str.length()+1, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
}

void MPIHelper::sendCheckpoint(Checkpoint *ckp, int dest) {
    stringstream ss;
    ckp->dump(ss);
    string str = ss.str();
    sendString(str, dest, TREE_TAG);
}


int MPIHelper::recvString(string &str, int src, int tag) {
    MPI_Status status;
    MPI_Probe(src, tag, MPI_COMM_WORLD, &status);
    int msgCount;
    MPI_Get_count(&status, MPI_CHAR, &msgCount);
    // receive the message
    char *recvBuffer = new char[msgCount];
    MPI_Recv(recvBuffer, msgCount, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
    str = recvBuffer;
    delete [] recvBuffer;
    return status.MPI_SOURCE;
}

int MPIHelper::recvCheckpoint(Checkpoint *ckp, int src) {
    string str;
    int proc = recvString(str, src, TREE_TAG);
    stringstream ss(str);
    ckp->load(ss);
    return proc;
}

void MPIHelper::broadcastCheckpoint(Checkpoint *ckp) {
    int msgCount = 0;
    stringstream ss;
    string str;
    if (isMaster()) {
        ckp->dump(ss);
        str = ss.str();
        msgCount = str.length()+1;
    }

    // broadcast the count for workers
    MPI_Bcast(&msgCount, 1, MPI_INT, PROC_MASTER, MPI_COMM_WORLD);

    char *recvBuffer = new char[msgCount];
    if (isMaster())
        memcpy(recvBuffer, str.c_str(), msgCount);

    // broadcast trees to workers
    MPI_Bcast(recvBuffer, msgCount, MPI_CHAR, PROC_MASTER, MPI_COMM_WORLD);

    if (isWorker()) {
        ss.clear();
        ss.str(recvBuffer);
        ckp->load(ss);
    }
    delete [] recvBuffer;
}

void MPIHelper::gatherCheckpoint(Checkpoint *ckp) {
    stringstream ss;
    ckp->dump(ss);
    string str = ss.str();
    int msgCount = str.length();

    // first send the counts to MASTER
    int *msgCounts = NULL, *displ = NULL;
    char *recvBuffer = NULL;
    int totalCount = 0;

    if (isMaster()) {
        msgCounts = new int[getNumProcesses()];
        displ = new int[getNumProcesses()];
    }
    MPI_Gather(&msgCount, 1, MPI_INT, msgCounts, 1, MPI_INT, PROC_MASTER, MPI_COMM_WORLD);

    // now real contents to MASTER
    if (isMaster()) {
        for (int i = 0; i < getNumProcesses(); i++) {
            displ[i] = totalCount;
            totalCount += msgCounts[i];
        }
        recvBuffer = new char[totalCount+1];
        memset(recvBuffer, 0, totalCount+1);
    }
    char *buf = (char*)str.c_str();
    MPI_Gatherv(buf, msgCount, MPI_CHAR, recvBuffer, msgCounts, displ, MPI_CHAR, PROC_MASTER, MPI_COMM_WORLD);

    if (isMaster()) {
        // now decode the buffer
        ss.clear();
        ss.str(recvBuffer);
        ckp->load(ss);

        delete [] recvBuffer;
        delete [] displ;
        delete [] msgCounts;
    }
}

IntVector MPIHelper::scheduleTasks(DoubleVector costs)
{
    int num_tasks = costs.size();
    int num_processes = getNumProcesses();

    std::vector<IntVector> process_tasks(num_processes);
    IntVector task_indices(num_tasks);
    for (int i = 0; i < num_tasks; ++i) {
        task_indices[i] = i; // Initialize task indices
    }
    // Sort tasks based on costs
    std::sort(task_indices.begin(), task_indices.end(),
              [&costs](int a, int b) { return costs[a] > costs[b]; });

    // Distribute tasks evenly across processes
    std::priority_queue<std::pair<double, int> > task_queue;
    for (int i = 0; i < getNumProcesses(); ++i) {
        task_queue.push({0.0, i}); // Initialize with zero cost for each process
    }
    for (int i = 0; i < num_tasks; ++i) {
        // Get the process with the least cost
        auto [current_cost, proc_id] = task_queue.top();
        task_queue.pop();
        
        // Assign the task to this process
        process_tasks[proc_id].push_back(task_indices[i]);

        // Update the cost for this process
        current_cost -= costs[task_indices[i]];
        task_queue.push({current_cost, proc_id});
    }

    // Now task_indices contains the process ID for each task
    return getProcVector(process_tasks);
}

DoubleVector MPIHelper::sumProcs(DoubleVector vals)
{
    int proc_size = vals.size();
    DoubleVector sum_vals(proc_size);
    MPI_Allreduce(vals.data(), sum_vals.data(), proc_size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    return sum_vals;
}

IntVector MPIHelper::getProcVector(const vector<IntVector> &vts)
{
    IntVector cnt_vt, offset_vt, flatten_vt;

    if (isMaster())
    {
        int offset = 0;
        for (const auto &vt : vts)
        {
            cnt_vt.push_back(vt.size());
            offset_vt.push_back(offset);
            offset += vt.size();
            flatten_vt.insert(flatten_vt.end(), vt.begin(), vt.end());
        }
    }

    int out_cnt;
    // Send cnt to each process
    MPI_Scatter(cnt_vt.data(), 1, MPI_INT, &out_cnt,
                1, MPI_INT, PROC_MASTER, MPI_COMM_WORLD);

    IntVector out_vt(out_cnt);
    // Send real contents to each process
    MPI_Scatterv(flatten_vt.data(), cnt_vt.data(), offset_vt.data(), MPI_INT,
                 out_vt.data(), out_cnt, MPI_INT, PROC_MASTER, MPI_COMM_WORLD);
    return out_vt;
}

vector<DoubleVector> MPIHelper::gatherAllVectors(vector<DoubleVector> &vts)
{
    if (getNumProcesses() == 1) {
        // If only one process, return the input vector directly
        return vts;
    }
    vector<DoubleVector> result(vts.size());
    Checkpoint *ckp = new Checkpoint();
    for (int i = 0; i < vts.size(); ++i) {
        if (!vts[i].empty()) {
            ckp->putVector(std::to_string(i), vts[i]);
        }
    }
    if (isWorker()) {
        sendCheckpoint(ckp, PROC_MASTER);    
        Checkpoint *recv_ckp = new Checkpoint();
        int src = recvCheckpoint(recv_ckp, PROC_MASTER);
        for (const auto &entry : *recv_ckp) {
            recv_ckp->getVector(entry.first, result[std::stoi(entry.first)]);
        }
        delete recv_ckp;
    } else {
        // Master process gathers all strings from workers
        for (int i = 1; i < getNumProcesses(); ++i) {
            Checkpoint *recv_ckp = new Checkpoint();
            int src = recvCheckpoint(recv_ckp, i);
            recv_ckp->transferSubCheckpoint(ckp, "");
            delete recv_ckp;
        }
        for (int i = 1; i < getNumProcesses(); ++i) {
            sendCheckpoint(ckp, i);
        }
        for (const auto &entry : *ckp) {
            ckp->getVector(entry.first, result[std::stoi(entry.first)]);
        }
    }
    delete ckp;
    return result;
}


vector<IntVector> MPIHelper::broadcastIntVectors(vector<IntVector> &vts)
{
    if (getNumProcesses() == 1) {
        // If only one process, return the input vector directly
        return vts;
    }
    vector<IntVector> result(vts.size());
    Checkpoint *ckp = new Checkpoint();
    
    if (isMaster()) {
        for (int i = 0; i < vts.size(); ++i) {
            if (!vts[i].empty()) {
                ckp->putVector(std::to_string(i), vts[i]);
            }
        }
        for (int i = 1; i < getNumProcesses(); ++i)
            sendCheckpoint(ckp, i);    
        result = vts; // Master process keeps its own vectors    
    } else {
        int src = recvCheckpoint(ckp, PROC_MASTER);
        if (ckp->empty()) {
            return vector<IntVector>();
        }
        for (const auto &entry : *ckp) {
            ckp->getVector(entry.first, result[std::stoi(entry.first)]);
        }
    }
    delete ckp;
    return result;
}


vector<DoubleVector> MPIHelper::broadcastDoubleVectors(vector<DoubleVector> &vts)
{
    if (getNumProcesses() == 1) {
        // If only one process, return the input vector directly
        return vts;
    }
    vector<DoubleVector> result(vts.size());
    Checkpoint *ckp = new Checkpoint();
    
    if (isMaster()) {
        for (int i = 0; i < vts.size(); ++i) {
            if (!vts[i].empty()) {
                ckp->putVector(std::to_string(i), vts[i]);
            }
        }
        for (int i = 1; i < getNumProcesses(); ++i)
            sendCheckpoint(ckp, i);    
        result = vts; // Master process keeps its own vectors    
    } else {
        int src = recvCheckpoint(ckp, PROC_MASTER);

        for (const auto &entry : *ckp) {
            vector<double> vec;
            ckp->getVector(entry.first, vec);
            result.push_back(vec);
        }
    }
    delete ckp;
    return result;
}

vector<string> MPIHelper::gatherStrings(const vector<string> &strs)
{
    if (getNumProcesses() == 1) {
        // If only one process, return the input vector directly
        return strs;
    }
    vector<string> result;
    Checkpoint *ckp = new Checkpoint();
    for (int i = 0; i < strs.size(); ++i) {
        if (!strs[i].empty()) {
            ckp->put(std::to_string(i), strs[i]);
        }
    }
    if (isWorker()) {
        sendCheckpoint(ckp, PROC_MASTER);    
        Checkpoint *recv_ckp = new Checkpoint();
        int src = recvCheckpoint(recv_ckp, PROC_MASTER);
        for (const auto &entry : *recv_ckp) {
            result.push_back(entry.second);
        }
        delete recv_ckp;
    } else {
        // Master process gathers all strings from workers
        for (int i = 1; i < getNumProcesses(); ++i) {
            Checkpoint *recv_ckp = new Checkpoint();
            int src = recvCheckpoint(recv_ckp, i);
            for (const auto &entry : *recv_ckp) {
                ckp->put(std::to_string(ckp->size()), entry.second);
            }
            delete recv_ckp;
        }
        for (int i = 1; i < getNumProcesses(); ++i) {
            sendCheckpoint(ckp, i);
        }
        for (const auto &entry : *ckp) {
            result.push_back(entry.second);
        }
    }
    delete ckp;
    return result;
}

void MPIHelper::syncCheckpoints(Checkpoint *ckp)
{
    if (getNumProcesses() == 1) {
        // If only one process, no need to sync
        return;
    }
    if (isMaster()) {
        for (int i = 1; i < getNumProcesses(); i++) {
            // receive model information from other processes
            Checkpoint *worker_ckp = new Checkpoint();
            int worker = MPIHelper::getInstance().recvCheckpoint(worker_ckp, i);
            ckp->putSubCheckpoint(worker_ckp, "");
        }
        for (int i = 1; i < MPIHelper::getInstance().getNumProcesses(); i++) {
            // send model information to other processes
            MPIHelper::getInstance().sendCheckpoint(ckp, i);
        }
    } else {
        MPIHelper::getInstance().sendCheckpoint(ckp, PROC_MASTER);
        MPIHelper::getInstance().recvCheckpoint(ckp, PROC_MASTER);
    }
}

vector<string> MPIHelper::gatherAllStrings(const vector<string> &strs)
{
    if (getNumProcesses() == 1) {
        // If only one process, return the input vector directly
        return strs;
    }
    vector<string> result(strs.size());
    Checkpoint *ckp = new Checkpoint();
    for (int i = 0; i < strs.size(); ++i) {
        if (!strs[i].empty()) {
            ckp->put(std::to_string(i), strs[i]);
        }
    }
    if (isWorker()) {
        sendCheckpoint(ckp, PROC_MASTER);    
        Checkpoint *recv_ckp = new Checkpoint();
        int src = recvCheckpoint(recv_ckp, PROC_MASTER);
        for (const auto &entry : *recv_ckp) {
            result[std::stoi(entry.first)] = entry.second;
        }
        delete recv_ckp;
    } else {
        // Master process gathers all strings from workers
        for (int i = 1; i < getNumProcesses(); ++i) {
            Checkpoint *recv_ckp = new Checkpoint();
            int src = recvCheckpoint(recv_ckp, i);
            for (const auto &entry : *recv_ckp) {
                ckp->put(entry.first, entry.second);
            }
            delete recv_ckp;
        }
        for (int i = 1; i < getNumProcesses(); ++i) {
            sendCheckpoint(ckp, i);
        }
        for (const auto &entry : *ckp) {
            result[std::stoi(entry.first)] = entry.second;
        }
    }
    delete ckp;
    return result;
}


#endif

MPIHelper::~MPIHelper() {
//    cleanUpMessages();
}

#ifdef _IQTREE_MPI
MPI_SharedWindow::MPI_SharedWindow(int num_elements)
    : window(MPI_WIN_NULL), shared_memory(nullptr), num_elements(num_elements), depth_lock(0) {
    MPI_Info win_info;
    MPI_Info_create(&win_info);

    // Create shared memory window for all processes
    MPI_Win_allocate_shared(MPIHelper::getInstance().isMaster() ? sizeof(double) * num_elements : 0, sizeof(double), win_info, MPI_COMM_WORLD, &shared_memory, &window);
    MPI_Info_free(&win_info);
    if (MPIHelper::getInstance().isMaster()) {
        // Initialize shared memory
        for (int i = 0; i < num_elements; i++) {
            shared_memory[i] = 0;
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    // Map shared memory for other processes
    if (MPIHelper::getInstance().isWorker()) {
        MPI_Aint size;
        int disp_unit;
        MPI_Win_shared_query(window, 0, &size, &disp_unit, &shared_memory);
    }
}

MPI_SharedWindow::~MPI_SharedWindow() {
    if (window != MPI_WIN_NULL) {
        MPI_Win_free(&window);  // Free the window before MPI_Finalize
    }
}

double MPI_SharedWindow::get_shared_memory(int idx) {
    assert(idx < num_elements);
    double ret;
    lock();
    MPI_Get(&ret, 1, MPI_DOUBLE, 0, idx, 1, MPI_DOUBLE, window);
    unlock();
    return ret;
}

void MPI_SharedWindow::set_shared_memory(int idx, double value) {
    assert(idx < num_elements);
    lock();
    MPI_Put(&value, 1, MPI_DOUBLE, 0, idx, 1, MPI_DOUBLE, window);
    unlock();
}

int MPI_SharedWindow::get_and_increment(int idx) {
    assert(idx < num_elements);
    double one = 1;
    double ret;
    lock();
    MPI_Fetch_and_op(&one, &ret, MPI_DOUBLE, 0, idx, MPI_SUM, window);
    unlock();
    return ret;
}

void MPI_SharedWindow::lock() {
    if (!depth_lock++)
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
}

void MPI_SharedWindow::unlock() {
    if (!--depth_lock)
        MPI_Win_unlock(0, window);
}

#endif
