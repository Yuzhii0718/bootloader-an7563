import os
import re




def find_secure_size(path_to_profile, parameter_name):
    with open(path_to_profile, 'r') as file:
        content = file.read()

    pattern = parameter_name + r'(\S+)'
    match = re.search(pattern, content)

    if match:
        macro_value = match.group(1)
        return macro_value
    else:
        return None



def modify_dtsi_reg_property(path_to_dtsi, node_name, new_reg_value):
    with open(path_to_dtsi, 'r') as file:
        content = file.read()

    pattern = r'(\s*' + node_name + r'\s*{\s*[\S\s]*?reg\s*=\s*<)(.*?)>([\S\s]*?};)'
    match = re.search(pattern, content)

    if match:
        original_reg_property = match.group(2)
        modified_reg_property = new_reg_value
        modified_content = content.replace(original_reg_property, modified_reg_property,1)

        with open(path_to_dtsi, 'w') as file:
            file.write(modified_content)

        return None
    else:
        return None








# main function


current_path = os.getcwd()
parent_path = os.path.dirname(current_path)

# ----------------------------------------------------------
src_path = os.path.join(parent_path, "tclinux_phoenix/Project/profile/")
dst_path = os.path.join(parent_path, "tclinux_phoenix/linux-ecnt/arch/arm/boot/dts")

#print("src path:", src_path)
#print("dst path:", dst_path)


# ----------------------------------------------------------
CUSTOM = os.environ["CUSTOM"]
PROFILE = os.environ["PROFILE"]

#print("custom:", CUSTOM)
#print("profile:", PROFILE)


# ----------------------------------------------------------
custom_path = os.path.join(src_path, CUSTOM+"/"+PROFILE)
src_file = os.path.join(custom_path, PROFILE+".profile")

dst_file = os.path.join(dst_path, "en7523.dtsi")

#print("src file:", src_file)
#print("dst file:", dst_file)


secure_dram_size = find_secure_size(src_file,"TCSUPPORT_OPTEE_SIZE=")


#print("secure_dram_size:", secure_dram_size)

modify_dtsi_reg_property (dst_file,'atf-reserved-memory@80000000','0x80000000 '+secure_dram_size)

