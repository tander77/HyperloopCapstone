import pickle
import os, sys

def main(argv):
    if len(argv) != 1:
        sys.exit("Incorrect usage. Expected: view_path.py <filename>")
    else:
        file_name = str(argv[0])
        if (os.path.isfile(file_name)):
            path = read_file_to_path(file_name)
            for x, y in path:
                print("x:", x, "y:", y)
        else:
            sys.exit("File not found")

def read_file_to_path(filename):
    with open(filename, "rb") as f:
        path = pickle.load(f)
    return path

if __name__ == "__main__":
   main(sys.argv[1:])