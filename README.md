# Project 1: Single-CPU Pre-emptive Process Scheduler Simulator

## Overview
This project simulates a single-CPU, multi-device, pre-emptive process scheduler to determine the optimal time quantum (TQ) that minimizes total process completion time for a given job-mix. It models I/O device priorities, context switching delays, and process scheduling using a round-robin approach with variable time quanta.

## Features
- Parses tracefiles defining devices and processes
- Simulates CPU scheduling with configurable time quanta
- Prioritizes I/O devices by transfer speed (fastest first)
- Handles context switches (5μs penalty) and data bus acquisition (5μs delay)
- Supports multi-blocked queues for concurrent I/O operations
- Evaluates job-mix performance across TQ ranges

## Requirements
- C99-compliant compiler (e.g., `gcc`)
- POSIX-compliant OS for compilation/execution

## Build & Run
```bash
# Compile
gcc -std=c99 -Wall -Werror -o besttq besttq.c

# Execute (basic)
./besttq <tracefile> <TQ-first> [<TQ-final> <TQ-increment>]

# Example
./besttq tracefile1 100 2000 100
```

## Usage
### Command Arguments
1. **Tracefile**: File describing devices and processes
2. **TQ-first**: Initial time quantum (μs)
3. **TQ-final** (Optional): Max TQ to test (default=TQ-first)
4. **TQ-increment** (Optional): Step size between TQs (default=1)

### Sample Output
```
best 1600 440800
```

## Tracefile Format
```
device <name> <bytes/sec>   # Define I/O device
reboot                      # End device definitions
process <ID> <start_time> { # Process definition
  i/o <exec_time> <device> <bytes>  # I/O request
  exit <total_exec_time>    # Process completion
}
```

### Example
```
device ssd 240000000
device hd 80000000
reboot
process 1 200 {
  i/o 100 hd 1600
  exit 400
}
```

## Key Implementation Details
1. **Device Prioritization**: Sorted by transfer speed (descending)
2. **Scheduling**:
   - Ready queue (FIFO)
   - Multi-blocked queues (per device priority)
3. **Time Accounting**:
   - Context switches: 5μs
   - Data bus acquisition: 5μs
   - I/O time = (bytes / speed) + bus acquisition time

## Limitations
- Maximum of 4 devices and 50 processes (compile-time configurable)
- I/O transfers use ceil() for microsecond rounding
- Single data bus with FIFO arbitration per priority tier
- Processes cannot overlap I/O operations

## Debugging Output
The program prints detailed state transitions during simulation. The final line always contains the optimal TQ and completion time.

## Contributors
- Owen Santoso (22466085)
- Victor Jongue (22493718)
