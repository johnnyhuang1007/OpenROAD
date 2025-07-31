set pdk_root  "/home/shawn/Bob/OpenROAD/testcase1"
set families  {SNPSHOPT25 SNPSLOPT25 SNPSROPT25 SNPSSLOPT25}

read_tech   "$pdk_root/testcase1.tf"

read_lef "$pdk_root/SNPSSLOPT25/lef/snps25slopt.lef"
read_lef "$pdk_root/SNPSROPT25/lef/snps25ropt.lef"
read_lef "$pdk_root/SNPSLOPT25/lef/snps25lopt.lef"
read_lef "$pdk_root/SNPSHOPT25/lef/snps25hopt.lef"

foreach fam $families {
  set libroot "$pdk_root/$fam/liberty/nldm"
  foreach grp {base cg iso pg dlvl ulvl ret} {
    # 只抓最慢角 (ss0p585v125c)；要全部都留就把 *ss0p585v125c.lib 拿掉
    foreach lib [glob -nocomplain "$libroot/$grp/*ss0p585v125c.lib"] {
      read_liberty $lib
    }
  }
}

# 直接保險：再手動讀一次最慢角 .lib（路徑請對應實際目錄）
# read_liberty "$pdk_root/SNPSSLOPT25/liberty/nldm/base/snps25slopt_base_ss0p585v125c.lib"

read_verilog "$pdk_root/testcase1.v"
link_design  top

read_def    -floorplan_initialize "$pdk_root/testcase1.def" 
#read_sdc     "$pdk_root/testcase2.sdc"

detailed_placement

write_def     snapshot.def
write_verilog snapshot.v
