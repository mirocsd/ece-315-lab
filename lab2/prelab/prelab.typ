= ECE 315 Lab 2 Prelab
== Miro Straszynski
\
1.
#image("Lab2Part1(1).png", width: 100%)
\

2.

In polling, we continously check the device's status in a loop, this check blocks the CPU temporarily. When a task is polling, we need to use a delay between checks to avoid hogging the CPU. An interrupt driven interface doesn't hog the CPU; the CPU remains in control and only triggers an interrupt handler when the appropriate event occurs. In comparison to polling, interrupts are more efficient because they do not take up a consistent amount of CPU time.
