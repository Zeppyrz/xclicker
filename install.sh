#!/bin/bash

set -e

base_dir="$(dirname "${BASH_SOURCE[0]}" | xargs realpath)"

APP_ID="xclicker"
BINARY="xclicker"
RELEASE_BINARY="${base_dir}/build/release/src/${BINARY}"
MO_FILE="${base_dir}/build/release/po/zh_CN/LC_MESSAGES/xclicker.mo"

desktop_file="${base_dir}/assets/${APP_ID}.desktop"
icon_file="${base_dir}/assets/icon.png"

usr_dir="$HOME/.local"
if [ "$(id -u)" -eq 0 ]; then
    usr_dir="/usr"
fi

bin_dir="${usr_dir}/bin"
share_dir="${usr_dir}/share"

help() {
    echo "Integrate XClicker with common desktop environments."
    echo
    echo "Usage: -i | --install    -- install desktop file, icon and binary"
    echo "       -u | --uninstall  -- uninstall desktop file, icon and binary"
    echo "       -h | --help       -- show usage"
}

install_app() {
    if [ ! -f "${RELEASE_BINARY}" ]; then
        echo "Error: Binary not found at ${RELEASE_BINARY}"
        echo "Please build first with: make release"
        exit 1
    fi

    echo "Installing binary to ${bin_dir}/${BINARY}"
    install -Dm755 "${RELEASE_BINARY}" "${bin_dir}/${BINARY}"

    echo "Installing desktop file to ${share_dir}/applications/${APP_ID}.desktop"
    install -Dm644 "${desktop_file}" "${share_dir}/applications/${APP_ID}.desktop"
    sed -i "s#Exec=xclicker#Exec=${bin_dir}/${BINARY}#g" "${share_dir}/applications/${APP_ID}.desktop"

    echo "Installing icon to ${share_dir}/icons/hicolor/256x256/apps/${APP_ID}.png"
    install -Dm644 "${icon_file}" "${share_dir}/icons/hicolor/256x256/apps/${APP_ID}.png"

    if [ -f "${MO_FILE}" ]; then
        echo "Installing translation to ${share_dir}/locale/zh_CN/LC_MESSAGES/xclicker.mo"
        install -Dm644 "${MO_FILE}" "${share_dir}/locale/zh_CN/LC_MESSAGES/xclicker.mo"
    fi

    echo "Updating desktop database"
    if command -v xdg-desktop-menu &>/dev/null; then
        xdg-desktop-menu forceupdate
    fi
    if command -v gtk-update-icon-cache &>/dev/null; then
        gtk-update-icon-cache -f -t "${share_dir}/icons/hicolor" &>/dev/null || true
    fi
}

uninstall_app() {
    echo "Removing binary from ${bin_dir}/${BINARY}"
    rm -f "${bin_dir}/${BINARY}"

    echo "Removing desktop file from ${share_dir}/applications/${APP_ID}.desktop"
    rm -f "${share_dir}/applications/${APP_ID}.desktop"

    echo "Removing icon from ${share_dir}/icons/hicolor/256x256/apps/${APP_ID}.png"
    rm -f "${share_dir}/icons/hicolor/256x256/apps/${APP_ID}.png"

    echo "Removing translation from ${share_dir}/locale/zh_CN/LC_MESSAGES/xclicker.mo"
    rm -f "${share_dir}/locale/zh_CN/LC_MESSAGES/xclicker.mo"

    echo "Updating desktop database"
    if command -v xdg-desktop-menu &>/dev/null; then
        xdg-desktop-menu forceupdate
    fi
    if command -v gtk-update-icon-cache &>/dev/null; then
        gtk-update-icon-cache -f -t "${share_dir}/icons/hicolor" &>/dev/null || true
    fi
}

while [[ "$#" -gt 0 ]]; do
    case $1 in
    -i | --install)
        install_app
        exit 0
        ;;
    -u | --uninstall)
        uninstall_app
        exit 0
        ;;
    -h | --help)
        help
        exit 0
        ;;
    *)
        echo "Unknown argument: $1"
        help
        exit 1
        ;;
    esac
done

help
