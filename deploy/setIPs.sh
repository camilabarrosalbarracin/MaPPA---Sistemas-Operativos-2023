#!/bin/bash
validate_ip() {
  # Regular expression pattern for IP address validation
  local ip_pattern='^([0-9]{1,3}.){3}[0-9]{1,3}$'

  if [[ $1 =~ $ip_pattern ]]; then
    return 0  # Valid IP address
  else
    return 1  # Invalid IP address
  fi
}

# Take the replace string
read -p "Ingrese Kernel IP: " ipKernel
read -p "Ingrese Memoria IP: " ipMemoria
read -p "Ingrese File System IP: " ipFS
read -p "Ingrese CPU IP: " ipCPU

# Regexs
sKer="IP_KERNEL=[0-9\.]*"
sMem="IP_MEMORIA=[0-9\.]*"
sFs="IP_FILESYSTEM=[0-9\.]*"
sCpu="IP_CPU=[0-9\.]*"

# Kernel
kc0="../Kernel/cfg/Kernel.config" 
kc1="../configs/Base/kernel_baseFIFO.config"
kc2="../configs/Base/kernel_baseRR.config"
kc3="../configs/Base/kernel_basePRIORIDADES.config"
kc4="../configs/Recursos/kernel_recursos.config"
kc5="../configs/Memoria/kernel_memoria.config"
kc6="../configs/FileSystem/kernel_fs.config"
kc7="../configs/Integral/kernel_integral.config"
kc8="../configs/Estres/kernel_estres.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc6
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc7
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc8
# Search and replace CPU
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc0
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc1
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc2
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc3
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc4
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc5
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc6
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc7
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc8
# Search and replace FS
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc0
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc1
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc2
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc3
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc4
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc5
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc6
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc7
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc8

# FS
fsc0="../FS/cfg/FS.config"
fsc1="../configs/Base/fs_base.config"
fsc2="../configs/Recursos/fs_recursos.config"
fsc3="../configs/Memoria/fs_memoria.config"
fsc4="../configs/FileSystem/fs_fs.config"
fsc5="../configs/Integral/fs_integral.config"
fsc6="../configs/Estres/fs_estres.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $fsc6

# CPU
cpc0="../CPU/cfg/CPU.config"
cpc1="../configs/Base/cpu_base.config"
cpc2="../configs/Recursos/cpu_recursos.config"
cpc3="../configs/Memoria/cpu_memoria.config"
cpc4="../configs/FileSystem/cpu_fs.config"
cpc5="../configs/Integral/cpu_integral.config"
cpc6="../configs/Estres/cpu_estres.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc6


# MEMORIA
mc0="../Memoria/cfg/Memoria.config"
mc1="../configs/Base/memoria_base.config"
mc2="../configs/Recursos/memoria_recursos.config"
mc3="../configs/Memoria/memoria_memoriaFIFO.config"
mc4="../configs/Memoria/memoria_memoriaLRU.config"
mc5="../configs/FileSystem/memoria_fs.config"
mc6="../configs/Integral/memoria_integral.config"
mc7="../configs/Estres/memoria_estres.config"

# Search and replace FS
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc0
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc1
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc2
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc3
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc4
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc5
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc6
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc7

echo "Las IPs han sido  modificadas correctamente"