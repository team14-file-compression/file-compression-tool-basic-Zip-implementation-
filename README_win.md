# File Compression Tool Basic ZIP Implementation

## Project Overview

This project is a standalone desktop utility operated via a Command Line Interface that compresses and decompresses files. It is developed strictly in C and C++. The core algorithm used for file size reduction is Huffman coding, which generates variable length codes based on character frequencies to compress data losslessly.

## Team 14 Contributors

* Pranavi Sistla (PES1UG24CS338) Project Lead and DevOps Engineer.


* Inchara K Kuppal (PES1UG25CS818) Developer and Agile Coordinator.


* Pavan Kumar H K (PES1UG24CS319) Lead Developer Compression.


* Rachit Upadhyay (PES1UG24CS355) Developer and QA Lead.



## Core Features

* Read an input file of any standard format.


* Compress the file utilizing Huffman coding to reduce its overall size.


* Output a compressed file with a custom .huff extension.


* Decompress a previously compressed file back to its exact original state.


* Display compression metrics such as original size, compressed size, and compression ratio.



## Build Instructions

Navigate to the project root directory in your terminal and run the make command. This will compile the source files located in the src folder using the headers in the include folder and generate the final executable. The project utilizes GitHub Actions for automated compilation and testing on every push.

## Usage Guide

The application runs directly from your terminal. Supply the appropriate flags to execute commands:

* For compression: Use the c flag followed by the file path.
* For decompression: Use the d flag followed by the file path.
* For custom output names: Use the o flag followed by the desired output path.
* For the help menu: Use the h flag.

## Project Structure

* src: Contains the core C++ source files for the CLI, file input, and Huffman logic.
* include: Contains the shared header files for cross module communication.
* tests: Contains unit tests for automated validation.
* Documents: Contains the project requirements, architecture, and testing plans.
