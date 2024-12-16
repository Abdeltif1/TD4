import os







#read adjacency matrix from file
def read_adjacency_matrix(file_name):
    with open(file_name, 'r') as f:
        lines = f.readlines()
    matrix = []
    for line in lines:
        matrix.append([int(x) for x in line.split()])
    return matrix


#print adjacency matrix 
def print_adjacency_matrix(matrix):
    for line in matrix:
        print(line)



adjacency_matrix = read_adjacency_matrix('adjacency.txt')
    
print_adjacency_matrix(adjacency_matrix)


def restructure(adjacency_matrix):
    new_matrix = []
    for i in range(len(adjacency_matrix)):
        temp = []
        for j in range(len(adjacency_matrix)):
            if adjacency_matrix[i][j] == 1:
                temp.append(j + 1)
        new_matrix.append(temp)

    return new_matrix

def array_to_format(array):
    result = "["
    for row in array:
        result += " ".join(str(x) for x in row)
        result += " ;"
    result = result[:-2]  # Remove the extra semicolon and space at the end
    result += "]"
    return result




def split_file(input_file, num_lines_per_file):
    with open(input_file, 'r') as f:
        lines = f.readlines()

    num_chunks = (len(lines) + num_lines_per_file - 1) // num_lines_per_file  # Ceiling division

    for i in range(num_chunks):
        chunk_start = i * num_lines_per_file
        chunk_end = (i + 1) * num_lines_per_file
        chunk = [line for line in lines[chunk_start:chunk_end] if line.strip()]  # Remove empty lines

        output_file = os.path.join('tables', f"table_{i+2}.txt")
        with open(output_file, 'w') as f_out:
            f_out.writelines(chunk)

    print(f"{num_chunks} files created.")

# print restructuring of adjacency matrix
# print(array_to_format(restructure(adjacency_matrix)))
split_file('table.txt', 11)