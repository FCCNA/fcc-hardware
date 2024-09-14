import yaml 

runs_info = {}
runs_info['BSO'] = {}
runs_info['BSO']['mu'] = {}
runs_info['BSO']['e'] = {}

range_info = {}
range_info['BSO'] = {}
range_info['BSO']['mu'] = {}
range_info['BSO']['e'] = {}

runs_info['BSO']['mu'][0] = 246 
runs_info['BSO']['mu'][20] = 247 
runs_info['BSO']['mu'][40] = 248 
runs_info['BSO']['mu'][50] = 249 
runs_info['BSO']['mu'][60] = 250 
runs_info['BSO']['mu'][70] = 251 
runs_info['BSO']['mu'][80] = 252 
runs_info['BSO']['mu'][90] = 253 
runs_info['BSO']['mu'][100] = 254 
runs_info['BSO']['mu'][110] = 255 
runs_info['BSO']['mu'][120] = 256 
runs_info['BSO']['mu'][130] = 257 
runs_info['BSO']['mu'][140] = 258 
runs_info['BSO']['mu'][160] = 259 
runs_info['BSO']['mu'][180] = 261 

runs_info['BSO']['e'][180] = 264
runs_info['BSO']['e'][160] = 265
runs_info['BSO']['e'][140] = 266
runs_info['BSO']['e'][130] = 267
runs_info['BSO']['e'][120] = 268
runs_info['BSO']['e'][110] = 269
runs_info['BSO']['e'][100] = 270
runs_info['BSO']['e'][90] = 271
runs_info['BSO']['e'][80] = 272
runs_info['BSO']['e'][70] = 273
runs_info['BSO']['e'][60] = 274
runs_info['BSO']['e'][50] = 275
runs_info['BSO']['e'][40] = 276
runs_info['BSO']['e'][20] = 277
runs_info['BSO']['e'][0] = 278




with open('data.yaml', 'w') as file:
    yaml.dump(runs_info, file, default_flow_style=False)
