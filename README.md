adhd
====

analyze diverse hardware demeanors (working title)
benchmark suite intended to highlight hardware tradeoffs

adhd started as the arraywalk program in the scrap repository; below the
original readme, until I have the time to create a new one reflecting all new
features and behaviours.

The arraywalk program creates an array and - you guessed it - walks through it.
This happens in a pre-encoded random fashion. Pre-encoded so individual
testcases can be run multiple times for a given array, which eliminates
variables when calculating average runtimes.
These tests are run for increasing array sizes, and several pre-encoded
patterns for each array size.
Overall, this should give a rather good impression about the influence of
caches in latency-optimized hardware (e.g. like a CPU, unlike a GPU).
