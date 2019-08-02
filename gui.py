import tkinter as tk
from tkinter import filedialog, messagebox
from compas import Kompas
from excel import Excel


class App(tk.Frame):
    def __init__(self, parent):
        tk.Frame.__init__(self, parent, background="white")
        self.parent = parent
        self.parent.title("Приложение")
        self.center_window()
        self.kompas = Kompas()
        self.excel = Excel()
        self.init_ui()
        self.pack(fill=tk.BOTH, expand=1)

    def get_excel_file(self, excel_label):
        file_name = filedialog.askopenfilename(initialdir="/", title="Select file",
                                               filetypes=(("Excel files", "*.xlsx"), ("", "*.xls")))
        if file_name == "":
            return 0
        self.excel.workbook = self.excel.api.WorkBooks.Open(file_name)
        name = self.excel.workbook.name
        if len(name) > 10:
            name = name[:6] + '... ' + name[-5:]
        excel_label.configure(text=name)

    def center_window(self):
        w = 420
        h = 170
        sw = self.parent.winfo_screenwidth()
        sh = self.parent.winfo_screenheight()
        x = (sw - w) / 2
        y = (sh - h) / 2
        self.parent.geometry('%dx%d+%d+%d' % (w, h, x, y))

    def init_ui(self):
        self.kompas.validation()
        self.excel.validation()

        self.columnconfigure(0)
        self.columnconfigure(1)
        self.columnconfigure(2)
        self.columnconfigure(3)
        self.rowconfigure(0, pad=30)
        self.rowconfigure(1, pad=30)
        self.rowconfigure(2, pad=30)

        exl_label = tk.Label(self, text="Текущий документ Excel:", bg="white")
        exl_label.grid(row=0, column=0, padx=30, sticky=tk.W)

        exl_file = tk.Label(self, text=self.excel.workbook, bg="white")
        exl_file.grid(row=0, column=1, columnspan=2)

        open_file_btn = tk.Button(self, text="Выбрать", command=lambda: self.get_excel_file(exl_file))
        open_file_btn.grid(row=0, column=3, padx=30)

        compas_label = tk.Label(self, text="Текущий документ Compas:", bg="white")
        compas_label.grid(row=1, column=0, padx=30, sticky=tk.W)

        compas_file = tk.Label(self, text=self.kompas.get_document_name(), bg="white")
        compas_file.grid(row=1, column=1, columnspan=2)

        refresh_btn = tk.Button(self, text="Обновить", command=lambda: self.reload_doc(compas_file))
        refresh_btn.grid(row=1, column=3, padx=30)

        export_btn = tk.Button(self, text="Экспорт в Excel", command=lambda: self.export_to_excel())
        export_btn.grid(row=2, column=0, columnspan=4)

    def reload_doc(self, compas_file_name):
        self.kompas.load_doc()
        name = self.kompas.get_document_name()
        compas_file_name.configure(text=name)


    def export_to_excel(self):
        try:
            lines = self.kompas.get_lines_projections()
            self.excel.paste_projections(lines)
        except KeyError:
            messagebox.showerror("Ошибка во время выполнения", "На активном виде отсутствуют отрезки")

def main():
    root = tk.Tk()
    root.resizable(width=False, height=False)
    App(root)
    root.mainloop()


if __name__ == '__main__':
    main()
