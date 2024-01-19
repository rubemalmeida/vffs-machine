(Get-Content vffs-webserver.ino) |
ForEach-Object {
    $_ -replace '^(.*)(const String hostname = ).*$', '${1}${2}SECRET_HOSTNAME' `
        -replace '^(.*)(const char \*ssid = ).*$', '${1}${2}SECRET_SSID' `
        -replace '^(.*)(const char \*password = ).*$', '${1}${2}SECRET_WIFI_PASSWORD'
} | Set-Content vffs-webserver.ino