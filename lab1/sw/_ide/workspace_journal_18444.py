# 2026-02-03T14:16:21.579755300
import vitis

client = vitis.create_client()
client.set_workspace(path="sw")

platform = client.create_platform_component(name = "lab1_platform",hw_design = "$COMPONENT_LOCATION/../../hw/ece315_lab1/lab1_hw_wrapper.xsa",os = "freertos",cpu = "ps7_cortexa9_0",domain_name = "freertos_ps7_cortexa9_0")

comp = client.create_app_component(name="lab1_part1",platform = "$COMPONENT_LOCATION/../lab1_platform/export/lab1_platform/lab1_platform.xpfm",domain = "freertos_ps7_cortexa9_0")

comp = client.get_component(name="lab1_part1")
status = comp.import_files(from_loc="", files=["C:\Users\miro\ece-315-lab\lab1\sw\part1\lab1_part1.c", "C:\Users\miro\ece-315-lab\lab1\sw\part1\pmodkypd.c", "C:\Users\miro\ece-315-lab\lab1\sw\part1\pmodkypd.h", "C:\Users\miro\ece-315-lab\lab1\sw\part1\ssd.c", "C:\Users\miro\ece-315-lab\lab1\sw\part1\ssd.h"])

platform = client.get_component(name="lab1_platform")
status = platform.build()

comp = client.get_component(name="lab1_part1")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

comp = client.create_app_component(name="lab1_part2",platform = "$COMPONENT_LOCATION/../lab1_platform/export/lab1_platform/lab1_platform.xpfm",domain = "freertos_ps7_cortexa9_0")

comp = client.get_component(name="lab1_part2")
status = comp.import_files(from_loc="", files=["C:\Users\miro\ece-315-lab\lab1\sw\part2\lab1_part2.c", "C:\Users\miro\ece-315-lab\lab1\sw\part2\pmodkypd.c", "C:\Users\miro\ece-315-lab\lab1\sw\part2\pmodkypd.h", "C:\Users\miro\ece-315-lab\lab1\sw\part2\pushbutton.c", "C:\Users\miro\ece-315-lab\lab1\sw\part2\pushbutton.h", "C:\Users\miro\ece-315-lab\lab1\sw\part2\rgb_led.c", "C:\Users\miro\ece-315-lab\lab1\sw\part2\rgb_led.h", "C:\Users\miro\ece-315-lab\lab1\sw\part2\vRgbTask.c"])

status = platform.build()

comp = client.get_component(name="lab1_part2")
comp.build()

status = platform.build()

comp = client.get_component(name="lab1_part1")
comp.build()

domain = platform.get_domain(name="freertos_ps7_cortexa9_0")

status = domain.set_config(option = "os", param = "freertos_tick_rate", value = "1000")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp = client.get_component(name="lab1_part2")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp = client.get_component(name="lab1_part1")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp = client.get_component(name="lab1_part2")
comp.build()

comp = client.get_component(name="lab1_part2")
status = comp.import_files(from_loc="$COMPONENT_LOCATION/../part1", files=["ssd.c", "ssd.h"], dest_dir_in_cmp = "lab1_part2")

status = platform.build()

comp = client.get_component(name="lab1_part2")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

comp = client.clone_component(name="lab1_part2",new_name="lab1_part3")

status = platform.build()

comp = client.get_component(name="lab1_part3")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

