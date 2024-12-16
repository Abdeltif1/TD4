import os


def truncate_file(file_path, comment):
    # Read the contents of the file
    with open(file_path, 'r') as file:
        lines = file.readlines()

    # Find the index of the specified comment
    comment_index = None
    for i, line in enumerate(lines):
        if comment in line:
            comment_index = i
            break

    if comment_index is not None:
        # Truncate the file to remove everything after the comment
        with open(file_path, 'w') as file:
            file.writelines(lines[:comment_index + 1])
        print(f"File '{file_path}' truncated successfully.")
    else:
        print(f"Comment '{comment}' not found in the file.")

#function to delete everything in a file
def delete_file(file_path):
    # Open the file in write mode
    with open(file_path, 'w') as file:
        # Clear the contents of the file
        file.truncate(0)
    print(f"File '{file_path}' cleared successfully.")

#functiono to delete everything in a directory but not the directory itself
def delete_directory_contents(directory_path):
    # Get a list of all files in the directory
    files = os.listdir(directory_path)

    # Iterate over all files and delete each one
    for file in files:
        file_path = os.path.join(directory_path, file)
        if os.path.isfile(file_path):
            os.remove(file_path)
            print(f"File '{file_path}' deleted successfully.")
        else:
            print(f"Skipping directory '{file_path}'.")

    print(f"All files in directory '{directory_path}' deleted successfully.")

# Example usage:
file_path = 'automate.c'  # Replace 'example.py' with the path to your Python file
comment = '/* Automated code */'
truncate_file(file_path, comment)

file_path = 'routes.txt'  # Replace 'example.py' with the path to your Python file
delete_file(file_path)

file_path = 'egress_ids.txt'  # Replace 'example.py' with the path to your Python file
delete_file(file_path)

file_path= 'vlan_logs'  # Replace 'example.py' with the path to your Python file
delete_directory_contents(file_path)
file_path= 'host_logs'  # Replace 'example.py' with the path to your Python file
delete_directory_contents(file_path)
file_path= 'tables'  # Replace 'example.py' with the path to your Python file
delete_directory_contents(file_path)