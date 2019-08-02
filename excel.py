from win32com.client import Dispatch


class Excel():
    def __init__(self):
        self.api = Dispatch('Excel.Application')
        self.workbook = "Не выбран"
        self.validation()

    def validation(self):
        if not self.api.Visible:
            self.api.Visible = True

    def paste_projections(self, lines_proj):
        k = 0
        sheet = self.workbook.ActiveSheet
        for line in lines_proj:
            sheet.Cells(k + 2, 1).value = line
            sheet.Cells(k + 2, 2).value = lines_proj[line]['x']
            sheet.Cells(k + 2, 3).value = lines_proj[line]['y']
            sheet.Cells(k + 2, 4).value = lines_proj[line]['z']
            k += 1