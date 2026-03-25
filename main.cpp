#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <functional>
#include <map>

#define APERIODIC_DEADLINE 500

struct Task {
	std::string id;
	int exec_time;
	int period;
};

struct ScheduleData {
	int num_p_tasks, num_a_tasks, exec_time;
	std::vector<Task> periodic_tasks;
	std::vector<Task> aperiodic_tasks;
};

struct Job {
    std::string id;
    int priority;
    int starttime;
    int remaining;
    int deadline;
};

struct Stats{
    std::map<std::string, int> preemptions;
	std::map<std::string, int> deadline_misses;

	void add_preemption(std::string id) {
		if (this->preemptions.find(id) != this->preemptions.end()) {
			this->preemptions[id]++;
		} else {
			this->preemptions[id] = 1;
		}
	}

	void add_deadline_miss(std::string id) {
		if (this->deadline_misses.find(id) != this->deadline_misses.end()) {
			this->deadline_misses[id]++;
		} else {
			this->deadline_misses[id] = 1;
		}
	}
};

void scheduler(ScheduleData* data, std::ofstream& out, std::function<bool(const Job&, const Job&)> job_compare) {
    std::vector<Job> jobs;
    Stats stat;

    Job* current = nullptr;
    Job* next_job = nullptr;

    std::string current_job_id = "";
    for (int t = 0; t < data->exec_time; t++) {

        // 1. check for deadline misses (now automatically checks BOTH types of tasks)
        for (int j = 0; j < jobs.size(); j++){
            if (jobs[j].remaining > 0 && jobs[j].deadline <= t){
                stat.add_deadline_miss(jobs[j].id);
                out << t << ": JOB " << jobs[j].id << " MISSED DEADLINE - REMAINING WORK: " << jobs[j].remaining << std::endl;
                jobs.erase(jobs.begin() + j);
                j--;
            }
        }

        // 2. release new periodic jobs
        for (int i = 0; i < data->periodic_tasks.size(); i++){
            if (t % data->periodic_tasks[i].period == 0){
                Job j;
                j.id = data->periodic_tasks[i].id;
                j.remaining = data->periodic_tasks[i].exec_time;
                j.deadline = t + data->periodic_tasks[i].period; 
                j.starttime = t;
                j.priority = data->periodic_tasks[i].period;

                jobs.push_back(j);
                out << t << ": JOB " << j.id << " QUEUED TO EXECUTE" << std::endl;
            }
        }

        // 2.5 release new aperiodic jobs
        for (int i = 0; i < data->aperiodic_tasks.size(); i++){
            // Checking arrival time
            if (t == data->aperiodic_tasks[i].period) {
                Job j;
                j.id = data->aperiodic_tasks[i].id;
                j.remaining = data->aperiodic_tasks[i].exec_time;
                j.starttime = t;
                j.deadline = t + APERIODIC_DEADLINE; 
                j.priority = APERIODIC_DEADLINE; 

                jobs.push_back(j);
                out << t << ": APERIODIC JOB " << j.id << " QUEUED TO EXECUTE" << std::endl;
            }
        }

        // 3. pick highest job ready
        if (jobs.size() > 0) {
            std::sort(jobs.begin(), jobs.end(), job_compare);    
            next_job = &jobs[0];
        } else {
            next_job = nullptr;
        }
        
        // 4. handle preemption
        if (next_job && current_job_id.length() > 0 && current_job_id != next_job->id) {
            stat.add_preemption(next_job->id);
            out << t << ": JOB " << next_job->id << " PREEMPTED " << current_job_id << std::endl;
        }

        // 5. execute job (-1 from remaining)
        if (next_job) {
            next_job->remaining--;
            current_job_id = next_job->id;
            
            // if job is complete, remove from active jobs
            if (next_job->remaining == 0) {
                out << t << ": JOB " << next_job->id << " FINISHED" << std::endl;
                jobs.erase(jobs.begin());
                next_job = nullptr;
                current_job_id = "";
                current = nullptr;
            }
        }

        current = next_job;
    }

	// Print the results
	out << "Scheduling Summary:" << std::endl;
	std::vector<Task> all_tasks;
	for (auto t : data->periodic_tasks) all_tasks.push_back(t);
	for (auto t : data->aperiodic_tasks) all_tasks.push_back(t);

	for (auto t : all_tasks) {
		out << "\t- Task " << t.id << ":" << std::endl;
		if (stat.preemptions.find(t.id) != stat.preemptions.end()) {
			out << "\t\t- Preemptions: " << stat.preemptions[t.id] << std::endl;
		} else {
			out << "\t\t- Preemptions: N/A" << std::endl;
		}

		if (stat.deadline_misses.find(t.id) != stat.deadline_misses.end()) {
			out << "\t\t- Deadline Misses: " << stat.deadline_misses[t.id];
		} else {
			out << "\t\t- Deadline Misses: N/A";
		}

		out << std::endl;
	}
}

void rma(ScheduleData* data, std::ofstream& out) {
    std::cout << "Running RMA Scheduling" << std::endl;
    out << std::endl << "====[ RMA SCHEDULING ]===" << std::endl;

    scheduler(data, out, [](const Job& a, const Job& b){
		return a.priority < b.priority;
	});
}

void edf(ScheduleData* data, std::ofstream& out) {
	std::cout << "Running EDF Scheduling" << std::endl;
	out << std::endl << "====[ EDF SCHEDULING ]===" << std::endl;

	scheduler(data, out, [](const Job& a, const Job& b){
		return a.deadline < b.deadline;
	});
}

void llf(ScheduleData* data, std::ofstream& out) {
	std::cout <<  "Running LLF Scheduling" << std::endl;
	out << std::endl << "====[ LLF SCHEDULING ]===" << std::endl;

	scheduler(data, out, [](const Job& a, const Job& b){
		int a_lax = a.deadline - a.remaining;
		int b_lax = b.deadline - b.remaining;
		return a_lax < b_lax;
	});
}

ScheduleData* read_input_file(char* filepath) {
	std::ifstream infile(filepath);

	int num_p_tasks, num_a_tasks;
	int exec_time;
	infile >> num_p_tasks;
	infile >> exec_time;

	ScheduleData* data = new ScheduleData;
	std::vector<Task> period_tasks, aperiodic_tasks;

	data->exec_time = exec_time;
	data->num_p_tasks = num_p_tasks;
	data->num_a_tasks = 0;
	data->periodic_tasks = period_tasks;
	data->aperiodic_tasks = aperiodic_tasks;
	
	// Periodic tasks
	for (int i = 0; i < num_p_tasks; i++) {
		std::string id, exec_time_str, period_str;

		infile >> id;
		infile >> exec_time_str;
		infile >> period_str;

		// Remove the commas in the first two entries
		id.pop_back();
		exec_time_str.pop_back();

		int task_exec_time = std::stoi(exec_time_str);
		int task_period = std::stoi(period_str);

		Task task;
		task.id = id;
		task.exec_time = task_exec_time;
		task.period = task_period;

		std::cout << "Added periodic task '" << id << "' with C=" << task_exec_time << ", T=" << task_period << std::endl;
		data->periodic_tasks.push_back(task);
	}

	// See if there are any aperiodic tasks
	if (infile >> num_a_tasks) {
		data->num_a_tasks = num_a_tasks;

		for (int i = 0; i < num_a_tasks; i++) {
			std::string id, exec_time_str, period_str;

			infile >> id;
			infile >> exec_time_str;
			infile >> period_str;

			// Remove the commas in the first two entries
			id.pop_back();
			exec_time_str.pop_back();

			int task_exec_time = std::stoi(exec_time_str);
			int task_period = std::stoi(period_str);

			Task task;
			task.id = id;
			task.exec_time = task_exec_time;
			task.period = task_period;

			std::cout << "Added aperiodic task '" << id << "' with C=" << task_exec_time << ", T=" << task_period << std::endl;
			data->aperiodic_tasks.push_back(task);
		}
	}

	infile.close();
	return data;
}

int main(int argc, char** argv)
{
    // Constrain to 3 arguments, otherwise send usage text and quit
    if (argc != 3) {
        std::cout << "Usage: ./scheduler.exe <input-file> <output-file>" << std::endl;
        return 1;
    }

    // Get the file paths to use
    char* input_file = argv[1];
    char* output_file = argv[2];

    // Create necessary information
    ScheduleData* data = read_input_file(input_file);
    std::ofstream output(output_file, std::ofstream::out | std::ofstream::trunc);

    // Run the scheduler for each algorithm
    rma(data, output);
    edf(data, output);
    llf(data, output);

    // Finalize the program, remove memory
    output.close();
    delete data;
    std::cout << "Simulation ending. Come again soon!" << std::endl;
    return 0;
}