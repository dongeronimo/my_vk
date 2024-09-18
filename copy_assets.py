import sys
import shutil
import os
script_name = sys.argv[0]
arguments = sys.argv[1:]
asset_folder = arguments[0]+"/build/assets";
#Delete the previous folder.
if os.path.exists(asset_folder):
   shutil.rmtree(asset_folder)
   print(f"{asset_folder} deleted.")
else:
   print(f"{asset_folder} does not exist.")
#copy the assets 
src_folder = arguments[0]+"/assets"
shutil.copytree(src_folder, asset_folder)
print("Assets copied")