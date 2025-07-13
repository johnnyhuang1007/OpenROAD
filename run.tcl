set pdk_root  "/home/johnny/OpenROAD/testcase1"
set families  {SNPSHOPT25 SNPSLOPT25 SNPSROPT25 SNPSSLOPT25}

read_lef -tech  "$pdk_root/case_tech_only.lef"


read_lef "/home/johnny/OpenROAD/testcase1/SNPSSLOPT25/lef/snps25slopt.lef"
read_lef "/home/johnny/OpenROAD/testcase1/SNPSROPT25/lef/snps25ropt.lef"
read_lef "/home/johnny/OpenROAD/testcase1/SNPSLOPT25/lef/snps25lopt.lef"
read_lef "/home/johnny/OpenROAD/testcase1/SNPSHOPT25/lef/snps25hopt.lef"


foreach fam $families {
  set libroot "$pdk_root/$fam/liberty/nldm"
  foreach grp {base cg iso pg dlvl ulvl ret} {
    foreach lib [glob -nocomplain "$libroot/$grp/*.lib"] {
      #if {($lib ne $min_lib) && ($lib ne $max_lib)} {
        read_liberty $lib
      #}
	  break
    }
  }
}


read_verilog "$pdk_root/testcase1.v"

link_design  top

read_def    -floorplan_initialize "$pdk_root/testcase1.def" 
read_sdc     "$pdk_root/testcase1.sdc"


write_def     snapshot.def
write_verilog snapshot.v