import sys
import shutil
import os
import subprocess
script_name = sys.argv[0]
arguments = sys.argv[1:]
shader_src_folder = arguments[0]+"/shaders"
shader_dst_folder = arguments[0]+"/build/shaders"
#Delete the previous folder.
if os.path.exists(shader_dst_folder):
   shutil.rmtree(shader_dst_folder)
   print(f"{shader_dst_folder} deleted.")
else:
   print(f"{shader_dst_folder} does not exist.")
#get all shaders in src
vertex_shaders = [f for f in os.listdir(shader_src_folder) if f.endswith(".vert")]
fragment_shaders = [f for f in os.listdir(shader_src_folder) if f.endswith(".frag")]
#create the folder
os.makedirs(shader_dst_folder, exist_ok=True)

#compile the shaders
for vs in vertex_shaders:
    print(f"compiling {vs}")
    arg = f"C:/VulkanSDK/1.3.268.0/Bin/glslc.exe {shader_src_folder}/{vs} -o {shader_dst_folder}/{vs}.spv"
    print(f">> {arg}");
    command = arg
    result = subprocess.run(command, capture_output=True, text=True)
    print(result.stdout)
for fs in fragment_shaders:
    print(f"compiling {fs}")
    arg = f"C:/VulkanSDK/1.3.268.0/Bin/glslc.exe {shader_src_folder}/{fs} -o {shader_dst_folder}/{fs}.spv"
    print(f">> {arg}");
    command = arg
    result = subprocess.run(command, capture_output=True, text=True)
    print(result.stdout)