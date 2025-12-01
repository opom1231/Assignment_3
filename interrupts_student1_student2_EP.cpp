/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_student1_student2.hpp"

void External_Priority_Scheduler(std::vector<PCB> &ready_queue){
    std::sort(
                ready_queue.begin(),
                ready_queue.end(),
                [](const PCB &first, const PCB &second){
                    if(first.priority != second.priority){
                        // Priority check (lower number is the higher priority)
                        return first.priority < second.priority;
                    }
                    // FCFS if the same priority 
                    return first.arrival_time < second.arrival_time;
                }
    );
}

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    // We need to initialize job_list with the input processes
    job_list = list_processes; 
    
    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        for(auto &process : job_list) {
            // Check if the process arrived 
            if(process.arrival_time <= current_time && (process.state == NEW || process.state == NOT_ASSIGNED)) {
                //if so, assign memory and put the process into the ready queue
                if(assign_memory(process)){
                    process.state = READY;  //Set the process state to READY
                    ready_queue.push_back(process); //Add the process to the ready queue

                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                } else {
                    // Memory is full, process remains in NEW state
                    process.state = NEW;
                }
                
            }
        }        

        // 2) Manage the wait queue
        // Check if we haeve processes ready
        if (running.PID == -1){
            if(!ready_queue.empty()){
                External_Priority_Scheduler(ready_queue);

                // Selecting the highest priority 
                running = ready_queue.front();
                ready_queue.erase(ready_queue.begin());

                running.state = RUNNING;
                sync_queue(job_list, running);

                execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            }

        }

        for(int i = 0; i < wait_queue.size(); i++){
            wait_queue[i].remaining_io_time--;

            if(wait_queue[i].remaining_io_time <= 0){
                // I/O is finished, add to ready queue and update job_list
                wait_queue[i].state = READY;
                ready_queue.push_back(wait_queue[i]);
                sync_queue(job_list, wait_queue[i]);

                execution_status += print_exec_status(current_time, wait_queue[i].PID, WAITING, READY);

                wait_queue.erase(wait_queue.begin() + i);
                i--;
            }
        }

        // 3) Schedule processes from the ready queue
        
        // Execute CPU
        if(running.PID != -1){
            running.remaining_time--;
            // Check for processes termination
            if(running.remaining_time <= 0){
                running.state = TERMINATED;
                free_memory(running);

                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, TERMINATED);
                
                idle_CPU(running);
            } else {
                // Check for I/O request 
                // Calculate how long the process has run
                int time_processed = running.processing_time - running.remaining_time;
                // Only trigger I/O if time is greater than 0 and it matches frequency interval
                if(running.io_freq > 0 && time_processed > 0 && time_processed % running.io_freq == 0){
                    running.state = WAITING;
                    running.remaining_io_time = running.io_duration;

                    wait_queue.push_back(running);
                    sync_queue(job_list, running);

                    execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, WAITING);

                    idle_CPU(running);
                }
            }

        }
        current_time++;

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}