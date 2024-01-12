#!/bin/bash
systemctl disable NetInfoDisplay.service && sync

cp ./NetInfoDisplay.service /etc/systemd/system/ && sync

systemctl enable NetInfoDisplay.service && sync

systemctl restart NetInfoDisplay.service && sync

