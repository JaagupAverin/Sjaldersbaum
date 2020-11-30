Remove-Item ".\Install" -Recurse -ErrorAction Ignore
New-Item -Path . -Name "Install\Sjaldersbaum" -ItemType "directory"

Copy-Item -Path ".\build\Release\Sjaldersbaum.exe" -Destination ".\Install\Sjaldersbaum"
Copy-Item -Path ".\build\Release\*" -Include "*.dll" -Destination ".\Install\Sjaldersbaum"
Copy-Item -Path ".\LICENSE.txt" -Destination ".\Install\Sjaldersbaum"

Copy-Item -Path ".\Sjaldersbaum\resources" -Destination ".\Install\Sjaldersbaum" -Recurse

Remove-Item -Path ".\Install\Sjaldersbaum" -Include "*.xcf" -Recurse
Remove-Item -Path ".\Install\Sjaldersbaum\resources\reminders" -Recurse

Compress-Archive -Path ".\Install\Sjaldersbaum" -DestinationPath ".\SjaldersbaumWin64.zip" -Force

Remove-Item ".\Install" -Recurse -ErrorAction Ignore