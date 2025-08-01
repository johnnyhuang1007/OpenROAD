source "helpers.tcl"

set blif [rmp::create_blif "" "" "" ""]
set db [ord::get_db]
read_lef "./Nangate45/Nangate45.lef"
read_def "./design_sequential.def"
read_liberty "./Nangate45/Nangate45_typ.lib"

set chip [$db getChip]
set block [$chip getBlock]


rmp::blif_add_instance $blif "_i1_"
rmp::blif_add_instance $blif "_i2_"
rmp::blif_add_instance $blif "_i3_"
rmp::blif_add_instance $blif "_r1_"


rmp::blif_dump $blif "./results/blif_writer_sequential.blif"

write_def "./results/design_seq.def"
diff_files "./results/blif_writer_sequential.blif" \
  "blif_writer_sequential.blif.ok"
