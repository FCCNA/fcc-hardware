import yaml 

runs_info = {}
runs_info['BSO'] = {}
runs_info['BGO'] = {}

runs_info['BSO']['mu'] = {}
runs_info['BSO']['e'] = {}


runs_info['BSO']['mu'][0] = [246] 
runs_info['BSO']['mu'][20] = [247] 
runs_info['BSO']['mu'][40] = [248] 
runs_info['BSO']['mu'][50] = [249] 
runs_info['BSO']['mu'][60] = [250] 
runs_info['BSO']['mu'][70] = [251] 
runs_info['BSO']['mu'][80] = [252] 
runs_info['BSO']['mu'][90] = [253] 
runs_info['BSO']['mu'][100] = [254] 
runs_info['BSO']['mu'][110] = [255] 
runs_info['BSO']['mu'][120] = [256] 
runs_info['BSO']['mu'][130] = [257] 
runs_info['BSO']['mu'][140] = [258] 
runs_info['BSO']['mu'][160] = [259] 
runs_info['BSO']['mu'][180] = [261] 

runs_info['BSO']['e'][180] = [264]
runs_info['BSO']['e'][160] = [265]
runs_info['BSO']['e'][140] = [266]
runs_info['BSO']['e'][130] = [267]
runs_info['BSO']['e'][120] = [268]
runs_info['BSO']['e'][110] = [269]
runs_info['BSO']['e'][100] = [270]
runs_info['BSO']['e'][90] = [271]
runs_info['BSO']['e'][80] = [272]
runs_info['BSO']['e'][70] = [273]
runs_info['BSO']['e'][60] = [274]
runs_info['BSO']['e'][50] = [275]
runs_info['BSO']['e'][40] = [276]
runs_info['BSO']['e'][20] = [277]
runs_info['BSO']['e'][0] = [278]


runs_info['BGO']['mu'] = {}

runs_info['BGO']['e'][180] = [207, 210, 211]
runs_info['BGO']['e'][160] = [213, 214]
runs_info['BGO']['e'][140] = [215, 216]
runs_info['BGO']['e'][130] = [217, 218]
runs_info['BGO']['e'][120] = [219, 220]
runs_info['BGO']['e'][110] = [221, 222]
runs_info['BGO']['e'][100] = [224, 225]
runs_info['BGO']['e'][90] = [205, 226, 227]
runs_info['BGO']['e'][80] = [228, 229, 230]
runs_info['BGO']['e'][70] = [231, 232]
runs_info['BGO']['e'][60] = [233, 234]
runs_info['BGO']['e'][50] = [235, 236]
runs_info['BGO']['e'][40] = [237, 240]
runs_info['BGO']['e'][20] = [239]
runs_info['BGO']['e'][0] = [244]


with open('data.yaml', 'w') as file:
    yaml.dump(runs_info, file, default_flow_style=False)
