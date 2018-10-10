if (Test-Path("proto.zip")) {
    Remove-Item proto.zip
}
Get-ChildItem -Recurse -Include *.proto | Compress-Archive -DestinationPath proto.zip