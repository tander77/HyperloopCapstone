import pickle
import os, sys

def main(argv):
    if len(argv) != 3:
        sys.exit("Incorrect usage. Expected: create_path.py max_x max_y <filename>")
    else:
        try:
            val1 = int(argv[0])
            try:
                val2 = int(argv[0])
                path_name = str(argv[2]) + '.txt'
                generate_path(int(argv[0]), int(argv[1]), path_name)
            except ValueError:
                sys.exit("Second argument must be int")
        except ValueError:
            sys.exit("First argument must be int")

        

# Define the path here as list of (x, y) tuples with x and y between 0 and 1.
def generate_path(max_count_x, max_count_y, path_name):
    path = []
    alter = False
    for y in range(max_count_y + 1):
        insert_y = y/max_count_y
        for x in range(max_count_x + 1):
            
            if alter:
                insert_x = (max_count_x - x)/max_count_x
                path.append((insert_x, insert_y))
            else:
                insert_x = x/max_count_x
                path.append((insert_x, insert_y))
        alter = not alter
    write_path_to_file(path, path_name)


def write_path_to_file(path, filename):
    with open(filename, "wb") as f:
        pickle.dump(path, f)

if __name__ == "__main__":
   main(sys.argv[1:])