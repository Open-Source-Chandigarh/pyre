import os

# --- CONFIGURATION ---
PROJECT_ROOT = "."  # Current directory

# Extensions to count
EXTENSIONS = {
    '.cpp', '.c', '.cc',    # C++ Source
    '.h', '.hpp',           # Headers
    '.vs', '.fs', '.glsl',  # Shaders
    '.py',                  # Python scripts
    '.cmake', 'CMakeLists.txt' # Build scripts
}

# Folders to completely ignore
IGNORE_DIRS = {
    'thirdparty',   # Don't count glad, imgui, stb, etc.
    'build',        # Don't count cmake build artifacts
    '.git',         # Git history
    '.vs',          # Visual Studio cache
    'out',          # Output binaries
    'resources'     # Assets
}

def count_lines_in_file(filepath):
    """Counts total lines and non-empty lines in a file."""
    total = 0
    code = 0
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                total += 1
                # Check if line has actual code (not just whitespace)
                if line.strip(): 
                    code += 1
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
    return total, code

def main():
    print(f"{'File Type':<15} | {'Files':<10} | {'Total LOC':<12} | {'Code (No Whitespace)':<20}")
    print("-" * 65)

    stats = {}  # Store stats per extension
    total_project_loc = 0
    total_project_code = 0
    total_files = 0

    for root, dirs, files in os.walk(PROJECT_ROOT):
        # Modify 'dirs' in-place to skip ignored directories
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]

        for file in files:
            ext = os.path.splitext(file)[1].lower()
            if file == "CMakeLists.txt": ext = "CMake"
            
            if ext in EXTENSIONS or file in EXTENSIONS:
                path = os.path.join(root, file)
                total, code = count_lines_in_file(path)

                if ext not in stats:
                    stats[ext] = {'files': 0, 'lines': 0, 'code': 0}
                
                stats[ext]['files'] += 1
                stats[ext]['lines'] += total
                stats[ext]['code'] += code

                total_project_loc += total
                total_project_code += code
                total_files += 1

    # Print Breakdown
    for ext, data in sorted(stats.items(), key=lambda x: x[1]['code'], reverse=True):
        print(f"{ext:<15} | {data['files']:<10} | {data['lines']:<12} | {data['code']:<20}")

    print("-" * 65)
    print(f"{'TOTAL':<15} | {total_files:<10} | {total_project_loc:<12} | {total_project_code:<20}")

if __name__ == "__main__":
    main()