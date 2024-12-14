# CSV to DB

A C program that efficiently loads CSV files into PostgreSQL databases using multi-threading capabilities. The program leverages the PostgreSQL libpq library and POSIX threads (pthreads) to parallelize the data loading process.

## Features

- CSV file processing
- Direct PostgreSQL database integration using libpq
- Support for large CSV files
- Configurable thread count for optimal performance

## Prerequisites

Before building the project, ensure you have the following installed:

### Linux
- GCC compiler
- PostgreSQL development files
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install gcc postgresql-server-dev-all

# Fedora
sudo dnf install gcc postgresql-devel

# Arch Linux
sudo pacman -S gcc postgresql
```

### macOS
- PostgreSQL (via Homebrew)
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install PostgreSQL
brew install libpq 
```

## Building the Project

### Linux and macOS
```bash
# Clone the repository
git clone https://github.com/M-Kouache/csv_to_db.git
cd csv_to_db
mkdir build && cd build
cmake ..
make
```

## Usage

2. Run the program:
```bash
./postigWriteChallenge <input_file.csv>
```

Example:
```bash
./postigWriteChallenge ../code-list.csv
```

## Troubleshooting

### Common Issues

1. Database Connection Errors
- Verify PostgreSQL is running
- Check credentials and connection settings
- Ensure database user has appropriate permissions

2. Build Errors
- Make sure all prerequisites are installed
- Check PostgreSQL development files location
- Verify PATH includes required binaries

### Platform-Specific Notes

#### Linux
- If libpq.h is not found, install postgresql-devel package
- Ensure pthreads development files are installed

#### macOS
- Check libpq installation via Homebrew


Hope you learned something :)

