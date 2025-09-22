Buffer Manager – Assignment 2
Overview

This project implements a Buffer Manager in C, which manages pages of a database file in memory.

It supports the following replacement strategies:

FIFO (First-In-First-Out)

LRU (Least Recently Used)

LRU-K (treated same as LRU for this assignment)

The Buffer Manager interacts with the Storage Manager (from Assignment 1) to read/write pages from disk.
It provides functionality to:

Pin/unpin pages

Mark pages dirty

Force pages to disk

Shutdown the buffer pool

Track statistics (fix counts, I/O reads/writes, dirty flags, etc.)

Project Structure
assign2_buffer_manager/
│── buffer_mgr.c        # Buffer Manager implementation
│── buffer_mgr.h        # Buffer Manager interface
│── buffer_mgr_stat.c   # Utility functions for debugging/testing
│── buffer_mgr_stat.h
│── storage_mgr.h       # Storage Manager header
│── dberror.c           # Error handling functions
│── dberror.h
│── dt.h                # Common datatypes (PageNumber, bool, etc.)
│── test_assign2_1.c    # Provided test cases for FIFO, LRU
│── test_assign2_2.c    # Provided test cases for LRU-K and error handling
│── test_helper.h       # Helper functions for testing
│── Makefile            # Build instructions

How to Compile

Open a terminal inside the assign2_buffer_manager folder and run:

make clean
make


If compilation is successful, you will see no errors or warnings.
This will generate two executables:

test_assign2_1

test_assign2_2

Expected outputs:

For test_assign2_1 : [test_assign2_1.c-Creating and Reading Back Dummy Pages-L111-21:31:01] OK: expected <Page-0> and was <Page-0>: reading back dummy page content
[test_assign2_1.c-Creating and Reading Back Dummy Pages-L111-21:31:01] OK: expected <Page-1> and was <Page-1>: reading back dummy page content
[test_assign2_1.c-Creating and Reading Back Dummy Pages-L111-21:31:01] OK: expected <Page-2> and was <Page-2>: reading back dummy page content
[test_assign2_1.c-Creating and Reading Back Dummy Pages-L111-21:31:01] OK: expected <Page-3> and was <Page-3>: reading back dummy page content
[test_assign2_1.c-Creating and Reading Back Dummy Pages-L111-21:31:01] OK: expected <Page-4> and was <Page-4>: reading back dummy page content
[test_assign2_1.c-Creating and Reading Back Dummy Pages-L111-21:31:01] OK: expected 
...........

For test_assign2_2 :[test_assign2_2.c-Testing LRU_K page replacement-L115-21:31:01] OK: expected <[0 0],[-1 0],[-1 0],[-1 0],[-1 0]> and was <[0 0],[-1 0],[-1 0],[-1 0],[-1 0]>: check pool content reading in pages
[test_assign2_2.c-Testing LRU_K page replacement-L115-21:31:01] OK: expected <[0 0],[1 0],[-1 0],[-1 0],[-1 0]> and was <[0 0],[1 0],[-1 0],[-1 0],[-1 0]>: check pool content reading in pages
[test_assign2_2.c-Testing LRU_K page replacement-L115-21:31:01] OK: expected <[0 0],[1 0],[2 0],[-1 0],[-1 0]> and was <[0 0],[1 0],[2 0],[-1 0],[-1 0]>: check pool content reading in pages
[test_assign2_2.c-Testing LRU_K page replacement-L115-21:31:01] OK: expected <[0 0],[1 0],[2 0],[3 0],[-1 0]> and was <[0 0],[1 0],[2 0],[3 0],[-1 0]>: check pool content reading in pages
[test_assign2_2.c-Testing LRU_K page replacement-L115-21:31:01] OK: expected <[0 0],[1 0],[2 0],[3 0],[4 0]> and was <[0 0],[1 0],[2 0],[3 0],[4 0]>: check pool content reading in pages
...........


