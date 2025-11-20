import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext
import subprocess
import os

class FileSorterGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("File Sorter")
        self.root.geometry("600x500")


        self.c_file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..","src", "sorter.c"))
        self.executable_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..","src", "sorter"))

        if not self.compile_c_program():
            messagebox.showerror("Error", "Failed to compile C program")
            self.root.destroy()
            return

        tk.Label(root, text="Source Folder:", font=("Arial", 12)).pack(pady=5)
        self.source_frame = tk.Frame(root)
        self.source_frame.pack(pady=5, padx=10, fill=tk.X)
        self.source_entry = tk.Entry(self.source_frame, width=50)
        self.source_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        tk.Button(self.source_frame, text="Browse", command=self.browse_source).pack(side=tk.LEFT, padx=5)

        tk.Label(root, text="Destination Folder:", font=("Arial", 12)).pack(pady=5)
        self.dest_frame = tk.Frame(root)
        self.dest_frame.pack(pady=5, padx=10, fill=tk.X)
        self.dest_entry = tk.Entry(self.dest_frame, width=50)
        self.dest_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        tk.Button(self.dest_frame, text="Browse", command=self.browse_destination).pack(side=tk.LEFT, padx=5)

        tk.Button(root, text="Start Sorting", command=self.sort_files, bg="green", fg="white", font=("Arial", 12, "bold"), height=2).pack(pady=20, padx=10, fill=tk.X)
        tk.Label(root, text="Output:", font=("Arial", 12)).pack(pady=5)
        self.output_text = scrolledtext.ScrolledText(root, height=15, width=70, state=tk.DISABLED)
        self.output_text.pack(pady=5, padx=10, fill=tk.BOTH, expand=True)

    def compile_c_program(self):
        if os.path.exists(self.executable_path):
            print("✓ sorter already compiled")
            return True
        try:
            print(f"Compiling {self.c_file_path}...")
            result = subprocess.run(
                ["gcc", "-o", self.executable_path, self.c_file_path],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                print("✓ Compilation successful!")
                return True
            else:
                print(f"✗ Compilation failed:\n{result.stderr}")
                messagebox.showerror("Compilation Error", result.stderr)
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            messagebox.showerror("Error", str(e))
            return False

    def browse_source(self):
        folder = filedialog.askdirectory(title="Select source folder")
        if folder:
            self.source_entry.delete(0, tk.END)
            self.source_entry.insert(0, folder)

    def browse_destination(self):
        folder = filedialog.askdirectory(title="Select destination folder")
        if folder:
            self.dest_entry.delete(0, tk.END)
            self.dest_entry.insert(0, folder)

    def sort_files(self):
        source = self.source_entry.get()
        destination = self.dest_entry.get()

        if not source or not destination:
            messagebox.showerror("Error", "Please select both source and destination folders")
            return

        if not os.path.exists(self.executable_path):
            messagebox.showerror("Error", f"C program executable not found: {self.executable_path}")
            return

        self.output_text.config(state=tk.NORMAL)
        self.output_text.delete(1.0, tk.END)
        self.output_text.config(state=tk.DISABLED)

        try:
            input_data = f"{source}\n{destination}\n"

            process = subprocess.Popen(
                [self.executable_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            stdout, stderr = process.communicate(input=input_data)

            self.output_text.config(state=tk.NORMAL)
            self.output_text.insert(tk.END, stdout)
            if stderr:
                self.output_text.insert(tk.END, f"\nErrors:\n{stderr}")
            self.output_text.config(state=tk.DISABLED)

            if process.returncode == 0:
                messagebox.showinfo("Success", "Files sorted successfully!")
            else:
                messagebox.showerror("Error", f"Program exited with code {process.returncode}")

        except Exception as e:
            messagebox.showerror("Error", f"Failed to run program: {str(e)}")

if __name__ == "__main__":
    root = tk.Tk()
    app = FileSorterGUI(root)
    root.mainloop()
