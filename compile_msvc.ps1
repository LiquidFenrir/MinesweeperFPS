$source_folders = "source", "glad/src", "imgui"
$inc_folders = "source", "glad/include", "imgui"

$extra_inc_folders = "enet64", "glfw3", "glm"
$libs_to_use = "enet64", "glfw3", "opengl32", "gdi32", "ws2_32", "winmm"

$build_folder = "build-msvc"
$data_folder = "data"

$output_file = "MinesweeperFPS.exe"


$obj_files_c = [System.Collections.ArrayList]@()
$source_files_c = [System.Collections.ArrayList]@()
$obj_files_cpp = [System.Collections.ArrayList]@()
$source_files_cpp = [System.Collections.ArrayList]@()
foreach ( $fol in $source_folders )
{
    $added_any = 0
    foreach ($fil in (get-childitem $fol -recurse))
    {
        $fp = $fol.OriginalPath
        $objpath = "$build_folder/$fol$fp/$fil.obj"
        $filpath = "$fol$fp/$fil"
        if($fil.extension -eq ".c")
        {
            [void]$obj_files_c.Add($objpath)
            [void]$source_files_c.Add($filpath)
            $added_any = 1
        }
        elseif($fil.extension -eq ".cpp")
        {
            [void]$obj_files_cpp.Add($objpath)
            [void]$source_files_cpp.Add($filpath)
            $added_any = 1
        }
    }
    if($added_any -eq 1)
    {
        [void](New-Item -Path "$build_folder/$fol" -Type Directory -Force)
    }
}

[void](New-Item -Path "$build_folder/$data_folder" -Type Directory -Force)
foreach ($datafil in (get-childitem $data_folder))
{
    if($datafil.extension -eq ".png" -or $datafil.extension -eq ".glsl")
    {
        $filpath = "$data_folder/$datafil"
        $outpath = "$build_folder/$filpath.h"
        & "py" "converter.py" $outpath $filpath
    }
}

$include_cmds = [System.Collections.ArrayList]@("$build_folder/$data_folder")
foreach ( $fol in $inc_folders )
{
    [void]$include_cmds.Add($fol)
}
foreach ( $fol in $extra_inc_folders)
{
    [void]$include_cmds.Add("extra-libs/$fol/include")
}

$lib_files = [System.Collections.ArrayList]@()
foreach ( $l in $libs_to_use )
{
    [void]$lib_files.Add("$l.lib")
}

Write-Host("C sources")
$source_files_c

Write-Host("C objs")
$obj_files_c

Write-Host("C++ sources")
$source_files_cpp

Write-Host("C++ objs")
$obj_files_cpp

Write-Host("includes")
$include_cmds

Write-Host("libraries")
$lib_files

Write-Host("Compiling .cpp files")
for ($index = 0; $index -lt $source_files_cpp.count; $index++)
{
    $fi = $source_files_cpp[$index]
    $fo = $obj_files_cpp[$index]

    $argslist = [System.Collections.ArrayList]@("/O2", "/std:c++17", "/EHsc")

    foreach ( $inc in $include_cmds)
    {
        [void]$argslist.Add("/I")
        [void]$argslist.Add($inc)
    }

    [void]$argslist.Add("/c")
    [void]$argslist.Add($fi)
    [void]$argslist.Add('/Fo"' + $fo + '"')

    & "cl.exe" $argslist
}

Write-Host("Compiling .c files")
for ($index = 0; $index -lt $source_files_c.count; $index++)
{
    $fi = $source_files_c[$index]
    $fo = $obj_files_c[$index]

    $argslist = [System.Collections.ArrayList]@("/O2")

    foreach ( $inc in $include_cmds)
    {
        [void]$argslist.Add("/I")
        [void]$argslist.Add($inc)
    }
    
    [void]$argslist.Add("/c")
    [void]$argslist.Add($fi)
    [void]$argslist.Add('/Fo"' + $fo + '"')

    & "cl.exe" $argslist
}

Write-Host("Linking")
$argslist = [System.Collections.ArrayList]@("/LIBPATH:extra-libs/libs-msvc-2019")
foreach ( $l in $lib_files)
{
    [void]$argslist.Add($l)
}
foreach ( $o in $obj_files_c)
{
    [void]$argslist.Add($o)
}
foreach ( $o in $obj_files_cpp)
{
    [void]$argslist.Add($o)
}
[void]$argslist.Add("/OUT:$output_file")

& "link.exe" $argslist
