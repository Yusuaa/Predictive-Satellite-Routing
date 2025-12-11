#!/bin/bash
set -e

echo "🔧 === COMPREHENSIVE QUAGGA PATCHING FOR DCE ==="

# ================================================================
# PATCH 1: Fix log.c - COMPREHENSIVE NULL pointer checks
# ================================================================
echo "🔧 Patching lib/log.c (comprehensive NULL fp protection)..."

LOG_C="/workspace/source/quagga/lib/log.c"

if [ -f "$LOG_C" ]; then
    cp "$LOG_C" "$LOG_C.bak"
    
    # Use awk to add NULL check at the beginning of vzlog function
    # The crash is at vzlog which uses zl->fp
    awk '
    /^vzlog[[:space:]]*\(/ { in_vzlog=1 }
    in_vzlog && /^{/ { 
        print $0
        print "  /* DCE NULL safety check - prevent fprintf to NULL */"
        print "  if (zl == NULL) return;"
        print "  if (zl->fp == NULL) return; /* Skip if no log file */"
        in_vzlog=0
        next
    }
    { print }
    ' "$LOG_C" > "${LOG_C}.new" && mv "${LOG_C}.new" "$LOG_C"
    
    # Add NULL check in time_print function (returns void, not int)
    awk '
    /^time_print[[:space:]]*\(/ || /^static.*time_print[[:space:]]*\(/ { in_func=1 }
    in_func && /^{/ { 
        print $0
        print "  /* DCE NULL safety */"
        print "  if (fp == NULL) return;"
        in_func=0
        next
    }
    { print }
    ' "$LOG_C" > "${LOG_C}.new" && mv "${LOG_C}.new" "$LOG_C"
    
    # Protect fprintf calls with NULL checks - be careful not to break syntax
    # Only protect simple patterns, not inside existing conditionals
    sed -i 's/^[[:space:]]*fprintf *(zl->fp,/  if (zl \&\& zl->fp) fprintf(zl->fp,/g' "$LOG_C"
    sed -i 's/^[[:space:]]*fprintf (fp,/  if (fp) fprintf(fp,/g' "$LOG_C"
    
    echo "✅ lib/log.c patched"
else
    echo "⚠️ lib/log.c not found at $LOG_C"
fi

# ================================================================
# PATCH 1b: Fix vty.c - NULL pointer checks in vty_read_config
# ================================================================
echo "🔧 Patching lib/vty.c..."

VTY_C="/workspace/source/quagga/lib/vty.c"

if [ -f "$VTY_C" ]; then
    cp "$VTY_C" "$VTY_C.bak"
    
    # Protect all fprintf calls with NULL stream checks
    sed -i 's/fprintf *(stderr,/if (stderr != NULL) fprintf(stderr,/g' "$VTY_C"
    sed -i 's/fprintf(stderr,/if (stderr != NULL) fprintf(stderr,/g' "$VTY_C"
    
    echo "✅ lib/vty.c patched"
else
    echo "⚠️ lib/vty.c not found"
fi

# ================================================================
# PATCH 1c: Fix command.c - Add NULL fp check in config_from_file
# ================================================================
echo "🔧 Patching lib/command.c..."

COMMAND_C="/workspace/source/quagga/lib/command.c"

if [ -f "$COMMAND_C" ]; then
    cp "$COMMAND_C" "$COMMAND_C.bak"
    
    # Find config_from_file function and add NULL check using awk
    awk '
    /^config_from_file/ { in_func=1 }
    in_func && /^{/ { 
        print $0
        print "  /* DCE NULL safety check */"
        print "  if (fp == NULL) { return CMD_SUCCESS; }"
        in_func=0
        next
    }
    { print }
    ' "$COMMAND_C" > "${COMMAND_C}.new" && mv "${COMMAND_C}.new" "$COMMAND_C"
    
    echo "✅ lib/command.c patched"
else
    echo "⚠️ lib/command.c not found"
fi

# ================================================================
# PATCH 1d: Fix vtysh_user.c - Disable authentication for DCE
# ================================================================
echo "🔧 Patching vtysh/vtysh_user.c (disable auth for DCE)..."

VTYSH_USER_C="/workspace/source/quagga/vtysh/vtysh_user.c"

if [ -f "$VTYSH_USER_C" ]; then
    cp "$VTYSH_USER_C" "$VTYSH_USER_C.bak"
    
    # Patch vtysh_auth to return immediately without checking passwd
    # The crash occurs because getpwuid returns NULL in DCE environment
    awk '
    /^vtysh_auth[[:space:]]*\(/ || /^void[[:space:]]+vtysh_auth[[:space:]]*\(/ { in_func=1 }
    in_func && /^{/ { 
        print $0
        print "  /* DCE PATCH: Skip authentication - getpwuid fails in DCE */"
        print "  return;"
        in_func=0
        next
    }
    { print }
    ' "$VTYSH_USER_C" > "${VTYSH_USER_C}.new" && mv "${VTYSH_USER_C}.new" "$VTYSH_USER_C"
    
    echo "✅ vtysh/vtysh_user.c patched (auth disabled for DCE)"
else
    echo "⚠️ vtysh/vtysh_user.c not found"
fi

# ================================================================
# PATCH 2: Fix pid_output.c - NULL path check
# ================================================================
echo "🔧 Patching lib/pid_output.c..."

PID_C="/workspace/source/quagga/lib/pid_output.c"

if [ -f "$PID_C" ]; then
    sed -i 's/pid = getpid/if (path == NULL) path = "\/var\/run\/quagga_default.pid";\n  pid = getpid/g' "$PID_C"
    echo "✅ pid_output.c patched"
else
    echo "⚠️ pid_output.c not found"
fi

# ================================================================
# PATCH 3: Fix dce-fd.cc - NULL path check  
# ================================================================
echo "🔧 Patching ns-3-dce/model/dce-fd.cc..."

DCE_FD="/workspace/source/ns-3-dce/model/dce-fd.cc"

if [ -f "$DCE_FD" ]; then
    sed -i 's#if (std::string (path) == "")#if (path == 0 || std::string (path) == "")#g' "$DCE_FD"
    echo "✅ dce-fd.cc patched"
else
    echo "⚠️ dce-fd.cc not found"
fi

# ================================================================
# PATCH 4: Create required directories
# ================================================================
echo "🔧 Creating required directories..."
mkdir -p /workspace/source/ns-3-dce/build/etc
mkdir -p /workspace/source/ns-3-dce/build/var/log
mkdir -p /workspace/source/ns-3-dce/build/var/run
mkdir -p /workspace/build/var/run
mkdir -p /var/run/quagga
chmod 755 /var/run/quagga

# ================================================================
# PATCH 5: Create Quagga configuration files in /etc/quagga
# ================================================================
echo "🔧 Creating Quagga configuration files..."

mkdir -p /etc/quagga
mkdir -p /workspace/source/ns-3-dce/build/etc/quagga

cat > /etc/quagga/zebra.conf << 'ZEBRAEOF'
hostname zebra
password zebra
enable password zebra
log stdout
!
interface lo
 ip address 127.0.0.1/32
!
line vty
 exec-timeout 0 0
!
ZEBRAEOF

cat > /etc/quagga/ospfd.conf << 'OSPFDEOF'
hostname ospfd
password zebra
enable password zebra
log stdout
!
router ospf
 ospf router-id 1.1.1.1
 network 10.0.0.0/8 area 0.0.0.0
!
line vty
 exec-timeout 0 0
!
OSPFDEOF

cp /etc/quagga/*.conf /workspace/source/ns-3-dce/build/etc/quagga/
chmod 644 /etc/quagga/*.conf

echo "✅ Quagga config files created"

# ================================================================
# REBUILD QUAGGA WITH DCE-COMPATIBLE FLAGS
# ================================================================
echo "🔧 Rebuilding Quagga with DCE-compatible flags..."
cd /workspace/source/quagga

# DCE requires: CFLAGS=-fPIC, LDFLAGS=-pie -rdynamic
# Note: -fPIC (Position Independent Code) NOT -fPIE
export CFLAGS="-fPIC -g -Wno-error"
export CXXFLAGS="-fPIC -g -Wno-error"
export LDFLAGS="-pie -rdynamic"

make clean 2>/dev/null || true

./configure \
    --disable-shared \
    --enable-static \
    --enable-vtysh \
    --enable-user=root \
    --enable-group=root \
    --with-cflags="-fPIC -g -Wno-error" \
    --localstatedir=/var/run \
    --sysconfdir=/etc/quagga

make -j$(nproc)

echo "📦 Copying rebuilt binaries..."
mkdir -p /workspace/source/ns-3-dce/build/bin_dce
mkdir -p /workspace/build/bin_dce

# Copy to multiple locations to ensure DCE can find them
cp zebra/zebra /workspace/source/ns-3-dce/build/bin_dce/
cp ospfd/ospfd /workspace/source/ns-3-dce/build/bin_dce/
cp vtysh/vtysh /workspace/source/ns-3-dce/build/bin_dce/

# Also copy to alternate locations
cp zebra/zebra /workspace/build/bin_dce/ 2>/dev/null || true
cp ospfd/ospfd /workspace/build/bin_dce/ 2>/dev/null || true
cp vtysh/vtysh /workspace/build/bin_dce/ 2>/dev/null || true

# Copy to system locations as backup
cp vtysh/vtysh /usr/bin/vtysh 2>/dev/null || true
cp vtysh/vtysh /usr/local/bin/vtysh 2>/dev/null || true

# Set permissions
chmod +x /workspace/source/ns-3-dce/build/bin_dce/*

echo "✅ Quagga rebuilt with DCE-compatible flags"

# ================================================================
# REBUILD DCE-QUAGGA MODULE
# ================================================================
echo "🔧 Rebuilding ns-3-dce..."
cd /workspace/source/ns-3-dce

rm -rf build/myscripts/ns-3-dce-quagga 2>/dev/null || true

./waf build

echo "✅ ns-3-dce rebuilt"

# ================================================================
# INSTALL GDB FOR DEBUGGING
# ================================================================
echo "🔧 Installing GDB..."
apt-get update -qq && apt-get install -y -qq gdb

# ================================================================
# CREATE DCE VIRTUAL FILESYSTEM CONFIG FILES
# ================================================================
echo "🔧 Creating DCE virtual filesystem config files for each node..."

cd /workspace/source/ns-3-dce

for i in $(seq 0 20); do
    node_dir="files-${i}"
    mkdir -p "${node_dir}/etc/quagga"
    mkdir -p "${node_dir}/var/run"
    mkdir -p "${node_dir}/var/log"
    mkdir -p "${node_dir}/tmp"
    
    router_id="1.1.1.${i}"
    
    cat > "${node_dir}/etc/quagga/zebra.conf" << ZEBRAEOF
hostname zebra
password zebra
enable password zebra
log stdout
!
interface lo
 ip address 127.0.0.1/32
!
line vty
 exec-timeout 0 0
!
ZEBRAEOF

    cat > "${node_dir}/etc/quagga/ospfd.conf" << OSPFDEOF
hostname ospfd
password zebra
enable password zebra
log stdout
!
router ospf
 ospf router-id ${router_id}
 network 10.0.0.0/8 area 0.0.0.0
!
line vty
 exec-timeout 0 0
!
OSPFDEOF

    # Create /etc/passwd and /etc/group for vtysh_auth()
    # vtysh needs these to lookup user info
    cat > "${node_dir}/etc/passwd" << PASSWDEOF
root:x:0:0:root:/root:/bin/bash
PASSWDEOF
    
    cat > "${node_dir}/etc/group" << GROUPEOF
root:x:0:
GROUPEOF

    chmod 644 "${node_dir}/etc/quagga/"*.conf
    chmod 644 "${node_dir}/etc/passwd"
    chmod 644 "${node_dir}/etc/group"
    echo "  Created config for node ${i} (router-id: ${router_id})"
done

echo "DCE virtual filesystem config files created"

# ================================================================
# RUN SIMULATION
# ================================================================
echo ""
echo "=== RUNNING SIMULATION ==="
echo ""

cd /workspace/source/ns-3-dce

if ./waf --run satnet-rfp 2>&1; then
    echo "Simulation completed successfully!"
    
    # Copy NetAnim trace file to mounted directory
    if [ -f "satnet-ospf-rfp-real-quagga.xml" ]; then
        echo "Copying NetAnim trace file to host..."
        cp satnet-ospf-rfp-real-quagga.xml myscripts/satnet-rfp/
        echo "Trace file available at: $(pwd)/myscripts/satnet-rfp/satnet-ospf-rfp-real-quagga.xml"
    else
        echo "NetAnim trace file not found!"
    fi
else
    echo "Simulation failed, running with GDB for more info..."
    ./waf --run satnet-rfp --command-template="gdb -batch -ex run -ex bt -ex quit --args %s"
fi
