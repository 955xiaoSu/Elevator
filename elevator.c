/*
    elevator code
    --------------------------------

    Written and maintained by 

    This code simulates the operation of an elevator serving a 33-story building.
*/

/*
    TODO：
    1. 扩展至 N 部电梯运行
    2. 扩展各类接口至集群部署、压力测试等
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define uint unsigned int
#define UP 1
#define DOWN 0
#define SHUTDOWN 1
#define OPENING 0
#define MAX_INPUT 13
#define TOP_FLOOR 33
#define LOWEST_FLOOR 0
#define RUNNING_SPEED 1
#define TIME_TO_CLOSE_DOOR 3

// #define DEBUG

struct Floor {
    int target_floor;
    struct Floor *next;
} *floor_up, *floor_down;

int curr_floor = 0, open_door_times = 0; // For synchronize

// "direction" parameter is used to pick up one of "floor_up" and "floor_down"
void moveReaction(const int target_floor, const uint direction) {
    if (target_floor < LOWEST_FLOOR || target_floor > TOP_FLOOR || 
        (direction != UP && direction != DOWN)) {
        fprintf(stderr, "floor: %d, direction: %d is an invalid operation\n", target_floor, direction);
        return;
    }

    sleep(RUNNING_SPEED);
    printf("Target floor: %d\t Current floor: %d\n", target_floor, curr_floor);

    if (direction == UP) {
        if (target_floor < curr_floor) {
            curr_floor -= 1;
            moveReaction(floor_up->target_floor, direction);
        } else if (target_floor > curr_floor) {
            curr_floor += 1;
            moveReaction(floor_up->target_floor, direction);
        } else {
            return;
        }
    } else {
        if (target_floor < curr_floor) {
            curr_floor -= 1;
            moveReaction(floor_down->target_floor, direction);
        } else if (target_floor > curr_floor) {
            curr_floor += 1;
            moveReaction(floor_down->target_floor, direction);
        } else {
            return;
        }
    }
}

void saveAndExit(int signum) {
    fflush(stdout);     // Display everything in buffer

    FILE *file = fopen("./initial_floor.txt", "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file for writing.\n");
    }
    fprintf(file, "%d", curr_floor);
    fclose(file);

    free(floor_up);
    free(floor_down);

    exit(EXIT_SUCCESS);
}

// First check whether there is any unfinished elevator lifting action in the current direction
// If not, add the current action directly
// If there is, we need to further process the position relationship according to the lifting logic
void liftSort(struct Floor* append_floor, const uint direction) {
    struct Floor* curr_ptr = NULL;
    struct Floor* next_ptr = NULL;
    if (direction == UP) {
        if (!floor_up) {
            floor_up = append_floor;
        } else {
            curr_ptr = next_ptr = floor_up;

            // Deal the situation where floor_up is equal to append_floor
            if (curr_ptr->target_floor == append_floor->target_floor)  return;

            // Deal the situation where floor_up is greater than append_floor
            if (curr_ptr->target_floor > append_floor->target_floor) {
                append_floor->next = floor_up;
                floor_up = append_floor;
                curr_ptr = next_ptr = NULL;
                return;
            }

            // Find the last element which is less than append_floor
            while (next_ptr->target_floor < append_floor->target_floor 
                    && next_ptr->next) {
                curr_ptr = next_ptr;
                next_ptr = next_ptr->next;
            }

            // situation: curr_ptr, append_floor, next_ptr(?)
            if (!next_ptr) {
                curr_ptr->next = append_floor;
            } else {
                if (next_ptr->target_floor > append_floor->target_floor) {
                    curr_ptr->next = append_floor;
                    append_floor->next = next_ptr;
                }
                else if (next_ptr->target_floor < append_floor->target_floor) {
                    next_ptr->next = append_floor;
                }
            }
            curr_ptr = next_ptr = NULL;
            return;
        }
    } else if (direction == DOWN) {
        if (!floor_down) {
            floor_down = append_floor;
        } else {
            curr_ptr = next_ptr = floor_down;

            // Deal the situation where floor_down is equal to append_floor
            if (curr_ptr->target_floor == append_floor->target_floor)  return;

            // Deal the situation where floor_down is greater than append_floor
            if (curr_ptr->target_floor < append_floor->target_floor) {
                append_floor->next = floor_down;
                floor_down = append_floor;
                curr_ptr = next_ptr = NULL;
                return;
            }

#ifdef DEBUG
    printf("\nCheck curr_ptr: %d, next_ptr: %d\n", curr_ptr->target_floor, next_ptr->target_floor);
#endif

            // Find the last element which is greater than append_floor
            while (next_ptr->target_floor > append_floor->target_floor 
                    && next_ptr->next) {
                curr_ptr = next_ptr;
                next_ptr = next_ptr->next;
            }

            // situation: curr_ptr, append_floor, next_ptr(?)
            if (!next_ptr) {
                curr_ptr->next = append_floor;
            } else {
                if (next_ptr->target_floor < append_floor->target_floor) {
                    curr_ptr->next = append_floor;
                    append_floor->next = next_ptr;
                }
                else if (next_ptr->target_floor > append_floor->target_floor) {
                    next_ptr->next = append_floor;
                }
#ifdef DEBUG
    printf("Check curr_ptr: %d, append_floor: %d, next_ptr: %d\n", curr_ptr->target_floor, append_floor->target_floor, next_ptr->target_floor);
#endif
            }
            curr_ptr = next_ptr = NULL;
            return;
        }
    } 
}

// Provide an user interface for receiving input
// Simulate somebody press on the button and wait for the elevator
void listenInput(int sign) {
    open_door_times++;

    int waiting_floor = 0;
    uint direction = 0;
    char buffer[MAX_INPUT];
    
    printf("\nEnter floor and direction (0 for down, 1 for up): ");

    // Be careful with the function like "scanf", because of the "%s" 
    // and the striction of function ended with "_s", it's better to use "fgets"
    fgets(buffer, sizeof(buffer), stdin); 
    if (sscanf(buffer, "%d %d", &waiting_floor, &direction) != 2) {
        fprintf(stderr, "Number of parameters does not match\n");
    } 
    if (waiting_floor < LOWEST_FLOOR || waiting_floor > TOP_FLOOR || 
        (direction != UP && direction != DOWN)) {
        fprintf(stderr, "floor: %d, direction: %d is an invalid operation\n", waiting_floor, direction);
        return;
    }

    // Initialize it before use it
    struct Floor* append_floor = malloc(sizeof(struct Floor));
    append_floor->target_floor = waiting_floor;
    liftSort(append_floor, direction);

#ifdef DEBUG
    printf("---------\nappend_floor: %d\n", append_floor->target_floor);
    if (direction == UP) {
        if (floor_up->next) {
            printf("floor_up: %d\t floor_up->next:%d\n---------\n", floor_up->target_floor, floor_up->next->target_floor);
        } else {
            printf("floor_up: %d\n---------\n", floor_up->target_floor);
        }
    } else {
        if (floor_down->next) {
            printf("floor_down: %d\t floor_down->next:%d\n---------\n", floor_down->target_floor, floor_down->next->target_floor);        
        } else {
            printf("floor_down: %d\n---------\n", floor_down->target_floor);        
        }
    } 
#endif

    return;
}

// Simulate somebody walk into the elevator and choose somewhere to go
void floorToGo(const uint direction) {
    open_door_times--;
    if (open_door_times < 0)  return;

    int goto_floor = 0;
    uint close_door = 0;
    char buffer[MAX_INPUT];
    
    printf("Enter floor and decide whether shutdown (0 for waiting, 1 for at once)\nYour choice: ");

    // Be careful with the function like "scanf", because of the "%s" 
    // and the striction of function ended with "_s", it's better to use "fgets"
    fgets(buffer, sizeof(buffer), stdin); 
    if (sscanf(buffer, "%d %d", &goto_floor, &close_door) != 2) {
        fprintf(stderr, "Number of parameters does not match\n");
    } 
    if (goto_floor < LOWEST_FLOOR || goto_floor > TOP_FLOOR || 
        (close_door != SHUTDOWN && close_door != OPENING)) {
        fprintf(stderr, "floor: %d, close_door: %d is an invalid operation\n", goto_floor, close_door);
        return;
    }

    // Simulate the delay of closing door
    if (close_door != SHUTDOWN) {
        sleep(TIME_TO_CLOSE_DOOR);
    }

    // Initialize it before use it
    struct Floor* append_floor = malloc(sizeof(struct Floor));
    append_floor->target_floor = goto_floor;
    liftSort(append_floor, direction);
    return;
}

void runTime(void) {
    printf("Current floor: %d\n", curr_floor);
    struct Floor *used_to_delete_pointer = NULL;
    
    while (1) {
        while (floor_up) {
            moveReaction(floor_up->target_floor, UP);
            floorToGo(UP);
            
            // In case of segmentation fault
            if (!floor_up->next)  {
                floor_up = NULL;
                break;
            }
            else {
                used_to_delete_pointer = floor_up;
                floor_up = floor_up->next;
                free(used_to_delete_pointer);
                used_to_delete_pointer = NULL;
            }
        }

        while (floor_down) {
            moveReaction(floor_down->target_floor, DOWN);
            floorToGo(DOWN);

            if (!floor_down->next) {
                floor_down = NULL;    
                break;
            }
            else {
                used_to_delete_pointer = floor_down;
                floor_down = floor_down->next;
                free(used_to_delete_pointer);
                used_to_delete_pointer = NULL;
            }
        }
    }
}

void getInitialFloor(void) {
    FILE *file = fopen("./initial_floor.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file for reading.\n");
        exit(EXIT_FAILURE);
    }
    if (fscanf(file, "%d", &curr_floor) != 1) {
        fprintf(stderr, "Error: Unable to read decimal number from file.\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

int main() {
    getInitialFloor();

    // Be careful with the signal, especially its object process
    if (signal(SIGINT, listenInput) == SIG_ERR) {
        fprintf(stderr, "Error: Unable to set signal handler for SIGINT.\n");
        return EXIT_FAILURE;
    }
    if (signal(SIGTSTP, saveAndExit) == SIG_ERR) {
        fprintf(stderr, "Error: Unable to set signal handler for SIGTSTP.\n");
        return EXIT_FAILURE;
    }

    floor_up = NULL;
    floor_down = NULL;
    // floor_up->target_floor = TOP_FLOOR;
    // floor_down->target_floor = LOWEST_FLOOR;
    
    runTime();
    return 0;
}