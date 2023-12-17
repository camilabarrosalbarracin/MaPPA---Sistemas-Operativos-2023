#!/bin/bash

echo "Seleccione el tipo de prueba a realizar:"
echo "1. Base"
echo "2. Recursos"
echo "3. Memoria"
echo "4. File System"
echo "5. Integral"
echo "6. Estres"

read -p "Ingrese el numero de la prueba: " prueba

case $prueba in
    1)
        echo "Prueba Base"
        cp "../configs/Base/memoria_base.config" "../Memoria/cfg/Memoria.config"
        cp "../configs/Base/cpu_base.config" "../CPU/cfg/CPU.config"
        cp "../configs/Base/fs_base.config" "../FS/cfg/FS.config"

        echo "1. FIFO"
        echo "2. RR"
        echo "2. PRIORIDADES"
        read -p "Ingrese el numero del algoritmo a usar: " algoritmo
        case $algoritmo in
            1)
                cp "../configs/Base/kernel_baseFIFO.config" "../Kernel/cfg/Kernel.config"
                ;;
            2)  
                cp "../configs/Base/kernel_baseRR.config" "../Kernel/cfg/Kernel.config"
                ;;
            3)
                cp "../configs/Base/kernel_basePRIORIDADES.config" "../Kernel/cfg/Kernel.config"
                ;;
        esac
        echo "Configuraciones modificadas correctamente para Prueba Base"
        ;;
    2)
        echo "Prueba Recursos"
        cp "../configs/Recursos/memoria_recursos.config" "../Memoria/cfg/Memoria.config"
        cp "../configs/Recursos/cpu_recursos.config" "../CPU/cfg/CPU.config"
        cp "../configs/Recursos/fs_recursos.config" "../FS/cfg/FS.config"
        cp "../configs/Recursos/kernel_recursos.config" "../Kernel/cfg/Kernel.config"
        echo "Configuraciones modificadas correctamente para Prueba Recursos"
        ;;
    3)
        echo "Prueba Memoria"
        cp "../configs/Memoria/cpu_memoria.config" "../CPU/cfg/CPU.config"
        cp "../configs/Memoria/kernel_memoria.config" "../Kernel/cfg/Kernel.config"
        cp "../configs/Memoria/fs_memoria.config" "../FS/cfg/FS.config"
        
        echo "1. FIFO"
        echo "2. LRU"
        read -p "Ingrese el numero del algoritmo a usar: " algMem
        case $algMem in
            1)
                cp "../configs/Memoria/memoria_memoriaFIFO.config" "../Memoria/cfg/Memoria.config"
                ;;
            2)  
                cp "../configs/Memoria/memoria_memoriaLRU.config" "../Memoria/cfg/Memoria.config"
                ;;   
        esac
        echo "Configuraciones modificadas correctamente para Prueba Memoria"
        ;;
    4)
        echo "Prueba File System"
        cp "../configs/FileSystem/memoria_fs.config" "../Memoria/cfg/Memoria.config"
        cp "../configs/FileSystem/cpu_fs.config" "../CPU/cfg/CPU.config"
        cp "../configs/FileSystem/fs_fs.config" "../FS/cfg/FS.config"
        cp "../configs/FileSystem/kernel_fs.config" "../Kernel/cfg/Kernel.config"
        echo "Configuraciones modificadas correctamente para File System"
        ;;
    5)
        echo "Prueba Integral"
        cp "../configs/Integral/memoria_integral.config" "../Memoria/cfg/Memoria.config"
        cp "../configs/Integral/cpu_integral.config" "../CPU/cfg/CPU.config"
        cp "../configs/Integral/fs_integral.config" "../FS/cfg/FS.config"
        cp "../configs/Integral/kernel_integral.config" "../Kernel/cfg/Kernel.config"
        echo "Configuraciones modificadas correctamente para Prueba Integral"
        ;;
    6)
        echo "Prueba Estres"
        cp "../configs/Estres/memoria_estres.config" "../Memoria/cfg/Memoria.config"
        cp "../configs/Estres/cpu_estres.config" "../CPU/cfg/CPU.config"
        cp "../configs/Estres/fs_estres.config" "../FS/cfg/FS.config"
        cp "../configs/Estres/kernel_estres.config" "../Kernel/cfg/Kernel.config"
        echo "Configuraciones modificadas correctamente para Prueba Estres"
        ;;
    *)
        echo "Comando incorrecto, intente de nuevo"
        ;;
esac

exit 0