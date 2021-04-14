#set_property IOSTANDARD LVCMOS33 [get_ports io1_o]
#set_property PACKAGE_PIN W20 [get_ports io0_o]
#set_property IOSTANDARD LVCMOS33 [get_ports io0_o]
#set_property IOSTANDARD LVCMOS33 [get_ports io0_t]
#set_property IOSTANDARD LVCMOS33 [get_ports io1_i]
#set_property IOSTANDARD LVCMOS33 [get_ports io1_t]
#set_property IOSTANDARD LVCMOS33 [get_ports ss_t]
#set_property IOSTANDARD LVCMOS33 [get_ports io0_i]
set_property IOSTANDARD LVCMOS33 [get_ports {ss_o[0]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {ss_i[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports MOSI]
set_property PACKAGE_PIN W20 [get_ports MOSI]

set_property IOSTANDARD LVCMOS33 [get_ports MISO]

set_property PACKAGE_PIN U18 [get_ports {ss_o[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports sck_o]
set_property PACKAGE_PIN W19 [get_ports sck_o]

set_property IOSTANDARD LVCMOS33 [get_ports {SD_reset[0]}]
set_property PACKAGE_PIN V20 [get_ports {SD_reset[0]}]


set_property PACKAGE_PIN V19 [get_ports MISO]

set_property IOSTANDARD LVCMOS33 [get_ports {SD_DAT1[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {SD_DAT2[0]}]
set_property PACKAGE_PIN T21 [get_ports {SD_DAT1[0]}]
set_property PACKAGE_PIN T20 [get_ports {SD_DAT2[0]}]
set_property PULLDOWN true [get_ports {SD_reset[0]}]
set_property PULLUP true [get_ports {SD_DAT1[0]}]
set_property PULLUP true [get_ports {SD_DAT2[0]}]
