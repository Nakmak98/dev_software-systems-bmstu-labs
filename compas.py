from win32com.client import Dispatch


class Kompas():
    def __init__(self):
        self.api5 = Dispatch('KOMPAS.Application.5')
        self.api7 = Dispatch('KOMPAS.Application.7')
        self.doc5 = self.doc7 = None
        self.load_doc()

    def load_doc(self):
        self.doc7 = self.api7.ActiveDocument
        self.doc5 = self.api5.ActiveDocument2D

    def validation(self):
        if not self.api5.Visible:
            self.api5.Visible = True

    def get_lines_coordinates(self):
        line_seg_coordinates = {}
        ksLineSegParam = 11
        ksLayerParam = 9
        iterator = self.api5.GetIterator
        layers = self.get_layers_num(iterator)
        line_seg_param = self.api5.GetParamStruct(ksLineSegParam)
        layer_param = self.api5.GetParamStruct(ksLayerParam)
        for layer in range(1, layers):
            lines_count = 0
            layer_obj_ptr = self.doc5.ksGetLayerReference(layer)
            self.doc5.ksGetObjParam(layer_obj_ptr, layer_param, -1)
            self.doc5.ksLayer(layer)
            line_segments = {}
            iterator.ksCreateIterator(1, layer_obj_ptr)
            obj = iterator.ksMoveIterator('F')
            while obj:
                self.doc5.ksGetObjParam(obj, line_seg_param, -1)
                line_segments.update({lines_count: {
                    'x1': line_seg_param.x1,
                    'x2': line_seg_param.x2,
                    'y1': line_seg_param.y1,
                    'y2': line_seg_param.y2
                }})
                obj = iterator.ksMoveIterator('N')
                lines_count += 1
            line_seg_coordinates.update({layer_param.name: line_segments})
            iterator.ksDeleteIterator()
        return line_seg_coordinates

    def get_layers_num(self, iterator):
        layers_num = 1
        ksLayer = 29
        iterator.ksCreateIterator(ksLayer, 0)
        iterator.ksMoveIterator('F')
        while iterator.ksMoveIterator('N'):
            layers_num += 1
        return layers_num

    def get_lines_projections(self):
        lines_coordinates = self.get_lines_coordinates()
        lines_projections = {}
        for i in lines_coordinates:
            lines_projections.update({i: {
                'x': self.get_projection(lines_coordinates[i], 'x'),
                'y': self.get_projection(lines_coordinates[i], 'y'),
                'z': self.get_projection(lines_coordinates[i], 'z'),
            }})
        return lines_projections

    def get_projection(self, lines, projection):
        try:
            if projection == 'x':
                return abs(round((lines[1]['x2'] - lines[1]['x1']) * 1e-3, 3))
            if projection == 'y':
                return abs(round((lines[0]['y2'] - lines[0]['y1']) * 1e-3, 3))
            if projection == 'z':
                return abs(round((lines[1]['y2'] - lines[1]['y1']) * 1e-3, 3))
        except KeyError:
            raise KeyError

    def get_document_name(self):
        try:
            name = self.doc7.name
        except AttributeError:
            return "Не выбран"
        if len(name) > 10:
            name = name[:6] + '... ' + name[-4:]
        return name

    def get_document_view(self):
        try:
            return self.doc7.ViewsAndLayersManager.Views.ActiveView.name
        except AttributeError:
            return "Не выбран"




