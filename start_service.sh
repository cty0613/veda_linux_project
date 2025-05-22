#!/bin/zsh

# 1) 바이너리 복사
sudo cp ./server /usr/local/bin/myserver
sudo chmod +x /usr/local/bin/myserver

# 2) 유닛 파일 생성
sudo tee /etc/systemd/system/myserver.service <<EOF
[Unit]
Description=My TCP & HTTP Server Daemon
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/myserver
WorkingDirectory=/home/cty0613/workspace/project
Restart=on-failure
RestartSec=5s
StandardOutput=journal
StandardError=journal
SyslogIdentifier=myserver

[Install]
WantedBy=multi-user.target
EOF

# 3) 등록·시작
sudo systemctl daemon-reload
sudo systemctl enable myserver.service
sudo systemctl start myserver.service

# 4) 동작 확인
sudo systemctl status myserver.service
