/*
    COMP3511 Fall 2024
    PA2: Multi-level Feedback Queue 
    
    Your name: LI,Yuntong 
    Your ITSC email: ylino@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Define MAX_NUM_PROCESS
// For simplicity, assume that we have at most 10 processes
#define MAX_NUM_PROCESS 10
#define MAX_PROCESS_NAME 5
#define MAX_GANTT_CHART 300

// N-level Feedback Queue (N=1,2,3,4)
#define MAX_NUM_QUEUE 4

// Keywords (to be used when parsing the input)
#define KEYWORD_QUEUE_NUMBER "queue_num"
#define KEYWORD_TQ "time_quantum"
#define KEYWORD_PROCESS_TABLE_SIZE "process_table_size"
#define KEYWORD_PROCESS_TABLE "process_table"

// Assume that we only need to support 2 types of space characters: 
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

// Process data structure
// Helper functions:
//  process_init: initialize a process entry
//  process_table_print: Display the process table
struct Process {
    char name[MAX_PROCESS_NAME];
    int arrival_time ;
    int burst_time;
    int remain_time; // remain_time is needed in the intermediate steps of MLFQ 
};
void process_init(struct Process* p, char name[MAX_PROCESS_NAME], int arrival_time, int burst_time) {
    strcpy(p->name, name);
    p->arrival_time = arrival_time;
    p->burst_time = burst_time;
    p->remain_time = 0;
}
void process_table_print(struct Process* p, int size) {
    int i;
    printf("Process\tArrival\tBurst\n");
    for (i=0; i<size; i++) {
        printf("%s\t%d\t%d\n", p[i].name, p[i].arrival_time, p[i].burst_time);
    }
}

//This is the helpful functions created by me 
struct Queue {
    int values[MAX_NUM_PROCESS];
    int front, rear, count;
};
void queue_init(struct Queue* q) {
    q->count = 0;
    q->front = 0;
    q->rear = -1;
}
int queue_is_empty(struct Queue* q) {
    return q->count == 0;
}
int queue_is_full(struct Queue* q) {
    return q->count == MAX_NUM_PROCESS;
}

int queue_peek(struct Queue* q) {
    return q->values[q->front];
}
void queue_enqueue(struct Queue* q, int new_value) {
    if (!queue_is_full(q)) {
        if ( q->rear == MAX_NUM_PROCESS -1)
            q->rear = -1;
        q->values[++q->rear] = new_value;
        q->count++;
        //printf("Enqueued %d, front: %d, rear: %d, count: %d\n", new_value, q->front, q->rear, q->count); //used for debugging, remember to comment it
    }
}
void queue_dequeue(struct Queue* q) {
	//printf("Dequeued %d, front: %d, rear: %d, count: %d\n", q->values[q->front], q->front, q->rear, q->count);//used for debugging, remember to comment it
    q->front++;
    if (q->front == MAX_NUM_PROCESS)
        q->front = 0;
    q->count--;
}
void queue_print(struct Queue* q) {
    int c = q->count;
    printf("size = %d\n", c);
    int cur = q->front;
    printf("values = ");
    while ( c > 0 ) {
        if ( cur == MAX_NUM_PROCESS )
            cur = 0;
        printf("%d ", q->values[cur]);
        cur++;
        c--;
    }
    printf("\n");
}


// A simple GanttChart structure
// Helper functions:
//   gantt_chart_print: display the current chart
struct GanttChartItem {
    char name[MAX_PROCESS_NAME];
    int duration;
};

void gantt_chart_update(struct GanttChartItem chart[MAX_GANTT_CHART], int* n, char name[MAX_PROCESS_NAME], int duration) {//自己加的 
    int i;
    i = *n;
    // The new item is the same as the last item
    if ( i > 0 && strcmp(chart[i-1].name, name) == 0) 
    {
        chart[i-1].duration += duration; // update duration
    } 
    else
    {
        strcpy(chart[i].name, name);
        chart[i].duration = duration;
        *n = i+1;
    }
}

void gantt_chart_print(struct GanttChartItem chart[MAX_GANTT_CHART], int n) {
    int t = 0;
    int i = 0;
    printf("Gantt Chart = ");
    printf("%d ", t);
    for (i=0; i<n; i++) {
        t = t + chart[i].duration;     
        printf("%s %d ", chart[i].name, t);
    }
    printf("\n");
}

// Global variables
int queue_num = 0;
int process_table_size = 0;
struct Process process_table[MAX_NUM_PROCESS];
int time_quantum[MAX_NUM_QUEUE];


// Helper function: Check whether the line is a blank line (for input parsing)
int is_blank(char *line) {
    char *ch = line;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) )
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
int is_skip(char *line) {
    if ( is_blank(line) )
        return 1;
    char *ch = line ;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}
// Helper: parse_tokens function
void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter) {
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Helper: parse the input file
void parse_input() {
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2]; // buffer for 2 tokens
    char *queue_tokens[MAX_NUM_QUEUE]; // buffer for MAX_NUM_QUEUE tokens
    int n;

    int numTokens = 0, i=0;
    char equal_plus_spaces_delimiters[5] = "";

    char process_name[MAX_PROCESS_NAME];
    int process_arrival_time = 0;
    int process_burst_time = 0;

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters,SPACE_CHARS);    

    // Note: MingGW don't have getline, so you are forced to do the coding in Linux/POSIX supported OS
    // In other words, you cannot easily coding in Windows environment

    while ( (nread = getline(&line, &len, fp)) != -1 ) {
        if ( is_skip(line) == 0)  {
            line = strtok(line,"\n");

            if (strstr(line, KEYWORD_QUEUE_NUMBER)) {
                // parse queue_num
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &queue_num);
                }
            } 
            else if (strstr(line, KEYWORD_TQ)) {
                // parse time_quantum
                parse_tokens(two_tokens, line, &numTokens, "=");
                if (numTokens == 2) {
                    // parse the second part using SPACE_CHARS
                    parse_tokens(queue_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(queue_tokens[i], "%d", &time_quantum[i]);
                    }
                }
            }
            else if (strstr(line, KEYWORD_PROCESS_TABLE_SIZE)) {
                // parse process_table_size
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &process_table_size);
                }
            } 
            else if (strstr(line, KEYWORD_PROCESS_TABLE)) {

                // parse process_table
                for (i=0; i<process_table_size; i++) {

                    getline(&line, &len, fp);
                    line = strtok(line,"\n");  

                    sscanf(line, "%s %d %d", process_name, &process_arrival_time, &process_burst_time);
                    process_init(&process_table[i], process_name, process_arrival_time, process_burst_time);

                }
            }

        }
        
    }
}
// Helper: Display the parsed values
void print_parsed_values() {
    printf("%s = %d\n", KEYWORD_QUEUE_NUMBER, queue_num);
    printf("%s = ", KEYWORD_TQ);
    for (int i=0; i<queue_num; i++)
        printf("%d ", time_quantum[i]);
    printf("\n");
    printf("%s = \n", KEYWORD_PROCESS_TABLE);
    process_table_print(process_table, process_table_size);
}

//This function is used for debugging
void debug_print_transition(int process_id, int from_level, int to_level) {  
    printf("Process P%d moved from Queue %d to Queue %d\n", process_id + 1, from_level, to_level);  
} 

 
// TODO: Implementation of MLFQ algorithm
void mlfq() {

    struct GanttChartItem chart[MAX_GANTT_CHART];
    int sz_chart = 0;
	
    // TODO: Write your code here to implement MLFQ
    // Tips: A simple array is good enough to implement a queue
	int total_burst_time = 0;  
    for (int i = 0; i < process_table_size; i++) {  
        process_table[i].remain_time = process_table[i].burst_time;  
        total_burst_time += process_table[i].burst_time;  
    }  

    // Initialize queues  
    struct Queue queues[queue_num];  
    int current_quantum[queue_num];  // Track current time quantum for each queue  
    for(int i = 0; i < queue_num; i++) {  
        queue_init(&queues[i]);  
        current_quantum[i] = 0;  
    }  
    
    int time = 0;  
    int completed = 0;  
    
    while (completed < total_burst_time) {
        // Check for new arrivals  
        for (int i = 0; i < process_table_size; i++) {  
            if (process_table[i].arrival_time == time && process_table[i].remain_time > 0) {  
                queue_enqueue(&queues[0], i);  
                //printf("time %d: %s arrives and is added to queue 0\n", time, process_table[i].name);  
            }  
        }  
		//printf("current_queue_count %d: \n", queues[0].count);  
		
        // Find the highest priority non-empty queue  
        int current_queue = -1;  
        for (int i = 0; i < queue_num; i++) {  
            if (!queue_is_empty(&queues[i])) {  
                current_queue = i;  
                break;  
            }  
        }  
		//printf("current_queue %d: \n", current_queue);  
		
        // If all queues are empty, handle idle time  
        if (current_queue == -1) {  
            gantt_chart_update(chart, &sz_chart, "idle", 1);  
            //printf("time %d: CPU idle\n", time);  
            time++;  
            completed++; 
            continue;  
        }  

        // Process execution  
        int current_process = queue_peek(&queues[current_queue]);  
        
        // Execute the process  
        process_table[current_process].remain_time--;  
        current_quantum[current_queue]++;  
        completed++;  
        
        gantt_chart_update(chart, &sz_chart, process_table[current_process].name, 1);  
        //printf("time %d: executing %s in queue %d\n", time, process_table[current_process].name, current_queue);  

        // Check if process is completed  
        if (process_table[current_process].remain_time == 0) {  
            queue_dequeue(&queues[current_queue]);  
            current_quantum[current_queue] = 0;  
            //printf("time %d: %s completed\n", time + 1, process_table[current_process].name);  
        }  
        // Check if time quantum expired  
        else if (current_quantum[current_queue] == time_quantum[current_queue]) {  
            if (current_queue < queue_num - 1) {  
                // Move to lower priority queue  
                int process = queue_peek(&queues[current_queue]);  
                queue_dequeue(&queues[current_queue]);  
                queue_enqueue(&queues[current_queue + 1], process);  
                //printf("time %d: %s moved from queue %d to queue %d\n", time + 1, process_table[process].name, current_queue, current_queue + 1);  
            }  
            current_quantum[current_queue] = 0;  
        }  
        time++;  
    }  
    // At the end, display the final Gantt chart
    gantt_chart_print(chart, sz_chart);

}


int main() {
    parse_input();
    print_parsed_values();
    mlfq();
    return 0;
}
