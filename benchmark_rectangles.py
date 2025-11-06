#!/usr/bin/env python3
"""
Benchmark script for RectangleAnimations example.
Runs the example with different rectangle counts and collects FPS data.
"""

import subprocess
import time
import os
import sys
import shutil

def main():
    # Configuration
    start_count = 3000
    end_count = 15000
    iterations = 5
    duration_seconds = 120
    engine = "gl"  # Can be "gl", "wg", or omit for software rendering
    
    # Calculate step size for 5 iterations
    step = (end_count - start_count) // (iterations - 1)
    rect_counts = [start_count + i * step for i in range(iterations)]
    
    print(f"Benchmark Configuration:")
    print(f"  Rectangle counts: {rect_counts}")
    print(f"  Duration per test: {duration_seconds} seconds")
    print(f"  Engine: {engine}")
    print(f"  Output directory: fps_logs/")
    print()
    
    # Create output directory
    os.makedirs("fps_logs", exist_ok=True)
    
    # Find the executable
    executable = None
    possible_paths = [
        "./build/examples/RectangleAnimations",
        "./build/bin/RectangleAnimations",
        "./RectangleAnimations",
        "./examples/RectangleAnimations"
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            executable = path
            break
    
    if not executable:
        print("Error: Could not find RectangleAnimations executable.")
        print("Please build the project first or specify the correct path.")
        print(f"Searched in: {possible_paths}")
        sys.exit(1)
    
    print(f"Found executable: {executable}\n")
    
    # Run benchmarks
    for i, rect_count in enumerate(rect_counts, 1):
        print(f"[{i}/{iterations}] Running with {rect_count} rectangles...")
        
        # Remove old fps_log.txt if it exists
        if os.path.exists("fps_log.txt"):
            os.remove("fps_log.txt")
        
        # Start the process
        cmd = [executable, engine, str(rect_count)]
        print(f"  Command: {' '.join(cmd)}")
        
        try:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Wait for the specified duration
            print(f"  Running for {duration_seconds} seconds...")
            time.sleep(duration_seconds)
            
            # Terminate the process
            process.terminate()
            
            # Wait a bit for graceful shutdown
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                print("  Process didn't terminate gracefully, killing...")
                process.kill()
                process.wait()
            
            print("  Process terminated.")
            
            # Wait a moment to ensure file is written and closed
            time.sleep(5)
            
            # Rename the fps_log.txt file
            if os.path.exists("fps_log.txt"):
                new_filename = f"fps_logs/fps_log_{rect_count}.txt"
                shutil.move("fps_log.txt", new_filename)
                print(f"  Saved FPS log to: {new_filename}")
                
                # Print summary statistics
                try:
                    with open(new_filename, 'r') as f:
                        lines = f.readlines()[1:]  # Skip header
                        if lines:
                            fps_values = [float(line.split(',')[1]) for line in lines if line.strip()]
                            if fps_values:
                                avg_fps = sum(fps_values) / len(fps_values)
                                min_fps = min(fps_values)
                                max_fps = max(fps_values)
                                print(f"  FPS Stats - Avg: {avg_fps:.2f}, Min: {min_fps:.2f}, Max: {max_fps:.2f}")
                except Exception as e:
                    print(f"  Warning: Could not parse FPS data: {e}")
            else:
                print("  Warning: fps_log.txt was not created")
            
            print()
            
        except Exception as e:
            print(f"  Error running benchmark: {e}")
            if 'process' in locals():
                try:
                    process.kill()
                except:
                    pass
            print()
            continue
    
    print("Benchmark complete!")
    print(f"All FPS logs saved in: fps_logs/")
    
    # Generate summary report
    print("\nGenerating summary report...")
    generate_summary_report(rect_counts)

def generate_summary_report(rect_counts):
    """Generate a summary CSV with all benchmark results."""
    summary_file = "fps_logs/benchmark_summary.csv"
    
    try:
        with open(summary_file, 'w') as out:
            out.write("RectCount,AvgFPS,MinFPS,MaxFPS,FrameCount\n")
            
            for rect_count in rect_counts:
                log_file = f"fps_logs/fps_log_{rect_count}.txt"
                if os.path.exists(log_file):
                    with open(log_file, 'r') as f:
                        lines = f.readlines()[1:]  # Skip header
                        if lines:
                            fps_values = [float(line.split(',')[1]) for line in lines if line.strip()]
                            if fps_values:
                                avg_fps = sum(fps_values) / len(fps_values)
                                min_fps = min(fps_values)
                                max_fps = max(fps_values)
                                frame_count = len(lines)
                                out.write(f"{rect_count},{avg_fps:.2f},{min_fps:.2f},{max_fps:.2f},{frame_count}\n")
        
        print(f"Summary report saved to: {summary_file}")
        
        # Print summary table
        print("\n" + "="*60)
        print("BENCHMARK SUMMARY")
        print("="*60)
        with open(summary_file, 'r') as f:
            print(f.read())
        print("="*60)
        
    except Exception as e:
        print(f"Warning: Could not generate summary report: {e}")

if __name__ == "__main__":
    main()

