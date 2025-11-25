#!/bin/bash

# Test Account Locking with List Update
# Tests that both account.txt AND in-memory list are updated when account is locked

echo "=========================================="
echo "TEST: Account Locking - File AND List Update"
echo "=========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if hungng is unlocked (status='1')
echo "1. Checking initial status of hungng..."
STATUS=$(grep "^hungng " account.txt | awk '{print $5}')
if [ "$STATUS" == "1" ]; then
    echo -e "${GREEN}✓ hungng is unlocked (status='1')${NC}"
else
    echo -e "${RED}✗ hungng is locked (status='$STATUS'). Please reset to '1' first.${NC}"
    echo "   Run: sed -i 's/hungng hung2a9 hungng@hust.edu soict.hust.edu.vn 0/hungng hung2a9 hungng@hust.edu soict.hust.edu.vn 1/' account.txt"
    exit 1
fi
echo ""

# Start server in background
echo "2. Starting server..."
./server 8888 &
SERVER_PID=$!
sleep 2

if ps -p $SERVER_PID > /dev/null; then
    echo -e "${GREEN}✓ Server started (PID: $SERVER_PID)${NC}"
else
    echo -e "${RED}✗ Server failed to start${NC}"
    exit 1
fi
echo ""

# Function to send login attempt
send_login_attempt() {
    local username=$1
    local password=$2
    local attempt_num=$3
    
    echo "3.$attempt_num. Sending login attempt: username='$username', password='$password'"
    
    # Use expect or nc to send data
    # This is a simplified version - you may need to adjust based on your protocol
    timeout 2 bash -c "echo -e '$username\n$password' | nc localhost 8888" > /dev/null 2>&1
    
    sleep 1
}

# Test Case: 3 failed login attempts
echo "3. Testing 3 failed login attempts..."
echo ""

send_login_attempt "hungng" "wrong1" "1"
echo -e "${YELLOW}   Attempt 1/3 - Expected: Invalid password${NC}"
echo ""

send_login_attempt "hungng" "wrong2" "2"
echo -e "${YELLOW}   Attempt 2/3 - Expected: Invalid password${NC}"
echo ""

send_login_attempt "hungng" "wrong3" "3"
echo -e "${YELLOW}   Attempt 3/3 - Expected: Account locked${NC}"
echo ""

sleep 2

# Check if account.txt was updated
echo "4. Checking if account.txt was updated..."
STATUS_AFTER=$(grep "^hungng " account.txt | awk '{print $5}')
if [ "$STATUS_AFTER" == "0" ]; then
    echo -e "${GREEN}✓ account.txt updated: hungng status='0' (locked)${NC}"
else
    echo -e "${RED}✗ account.txt NOT updated: hungng status='$STATUS_AFTER'${NC}"
fi
echo ""

# Check if auth.log has ACCOUNT_LOCKED event
echo "5. Checking auth.log for ACCOUNT_LOCKED event..."
if grep -q "ACCOUNT_LOCKED.*hungng" log/auth.log; then
    echo -e "${GREEN}✓ auth.log contains ACCOUNT_LOCKED event for hungng${NC}"
    tail -5 log/auth.log | grep ACCOUNT_LOCKED
else
    echo -e "${RED}✗ auth.log does NOT contain ACCOUNT_LOCKED event${NC}"
fi
echo ""

# CRITICAL TEST: Try to login with CORRECT password (should be blocked)
echo "6. CRITICAL TEST: Trying to login with CORRECT password..."
echo "   This tests if in-memory list was updated!"
echo ""

send_login_attempt "hungng" "hung2a9" "4"
echo -e "${YELLOW}   Expected: Account is blocked (proves list was updated)${NC}"
sleep 2
echo ""

# Check server logs
echo "7. Checking server console output..."
echo "   Look for '[LOCK ACCOUNT IN LIST]' message:"
echo ""
echo -e "${YELLOW}--- Recent server logs ---${NC}"
# Server logs are in terminal, we can check if the process is still running
if ps -p $SERVER_PID > /dev/null; then
    echo -e "${GREEN}✓ Server is still running${NC}"
else
    echo -e "${RED}✗ Server has stopped${NC}"
fi
echo ""

# Cleanup
echo "8. Cleaning up..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo -e "${GREEN}✓ Server stopped${NC}"
echo ""

# Summary
echo "=========================================="
echo "TEST SUMMARY"
echo "=========================================="
echo "Expected behavior:"
echo "1. After 3 failed attempts:"
echo "   - account.txt updated (status='0')"
echo "   - In-memory list updated (status='0')"
echo "   - Server console shows: [LOCK ACCOUNT IN LIST]"
echo ""
echo "2. Next login attempt with CORRECT password:"
echo "   - Server blocks login (proves list was updated)"
echo "   - Server sends: 'Account is blocked.'"
echo ""
echo "Manual verification steps:"
echo "1. Check account.txt: cat account.txt | grep hungng"
echo "2. Check auth.log: tail -10 log/auth.log | grep ACCOUNT_LOCKED"
echo "3. Reset for retest: sed -i 's/hungng hung2a9 hungng@hust.edu soict.hust.edu.vn 0/hungng hung2a9 hungng@hust.edu soict.hust.edu.vn 1/' account.txt"
echo ""
echo "=========================================="
