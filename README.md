# Pattern-Matching-Algorithm-With-CSA
Improving pattern-matching algorithm using counting set automata

# Counting Set Automata Regex Matching

Implementation of a regular expression matching algorithm based on  
**Counting Set Automata (CSA)**.

Traditional regex engines rely on backtracking, which can lead to  
exponential runtime when handling patterns with nested quantifiers  
(e.g., catastrophic backtracking or ReDoS attacks).

This project implements a CSA-based regex matcher in C, designed to  
maintain near-linear matching complexity regardless of repetition count.

## Features

- Counting Automata and Counting Set Automata implementation
- Regex parsing and automata construction
- Optimization techniques for repeated symbols and patterns
- Performance comparison with traditional backtracking engines

## Result

Experiments show that CSA maintains **linear growth in matching steps**,  
while backtracking-based engines exhibit **non-linear runtime increases**.

## Language

C
