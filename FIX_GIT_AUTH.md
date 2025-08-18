# Fix Git Authentication for GitHub

## Problem
GitHub no longer accepts password authentication for Git operations. You need a Personal Access Token (PAT).

## Solution 1: Personal Access Token (Recommended)

### Step 1: Create a Personal Access Token on GitHub
1. Go to https://github.com/settings/tokens
2. Click "Generate new token" → "Generate new token (classic)"
3. Give it a name like "Phoenix-Chimera-Push"
4. Select scopes:
   - ✅ repo (all)
   - ✅ workflow (if using GitHub Actions)
5. Click "Generate token"
6. **COPY THE TOKEN NOW** (you won't see it again!)

### Step 2: Update Git to Use Token
Run this command and use your token as the password when prompted:
```bash
git push origin main
# Username: blewistrumpet-lang
# Password: [PASTE YOUR TOKEN HERE]
```

### Step 3: Save Token for Future Use (Optional)
To avoid entering the token every time:
```bash
git config --global credential.helper osxkeychain
```
Then push once with the token, and macOS will save it.

## Solution 2: Use SSH Instead (Alternative)

### Step 1: Check for Existing SSH Key
```bash
ls -la ~/.ssh/id_*
```

### Step 2: Generate SSH Key (if needed)
```bash
ssh-keygen -t ed25519 -C "blewistrumpet@gmail.com"
# Press Enter for default location
# Enter a passphrase (optional)
```

### Step 3: Add SSH Key to GitHub
1. Copy your public key:
```bash
cat ~/.ssh/id_ed25519.pub
```
2. Go to https://github.com/settings/keys
3. Click "New SSH key"
4. Paste the key and save

### Step 4: Change Remote to SSH
```bash
git remote set-url origin git@github.com:blewistrumpet-lang/phoenix-Chimera.git
```

### Step 5: Test and Push
```bash
ssh -T git@github.com
git push origin main
```

## Solution 3: GitHub CLI (Easiest)

### Install GitHub CLI
```bash
brew install gh
```

### Authenticate
```bash
gh auth login
# Choose: GitHub.com
# Choose: HTTPS
# Choose: Login with web browser
# Follow prompts
```

### Push
```bash
git push origin main
```

## Quick Fix (Temporary)
If you need to push immediately and have a token:
```bash
git push https://blewistrumpet-lang:YOUR_TOKEN@github.com/blewistrumpet-lang/phoenix-Chimera.git main
```
⚠️ Don't save this command in history!

## Verify Current Setup
```bash
# Check current remote
git remote -v

# Test connection (for SSH)
ssh -T git@github.com

# Check saved credentials
git config credential.helper
```

## Troubleshooting

### If token is rejected:
- Make sure you selected the 'repo' scope when creating token
- Check token hasn't expired
- Ensure you're using token, not GitHub password

### If SSH fails:
- Check SSH agent is running: `eval "$(ssh-agent -s)"`
- Add key to agent: `ssh-add ~/.ssh/id_ed25519`
- Check GitHub has your public key

### Clear saved credentials (if needed):
```bash
# Clear git credentials from keychain
git credential-osxkeychain erase
host=github.com
protocol=https
[press enter twice]
```

## Current Repository
- Repo: https://github.com/blewistrumpet-lang/phoenix-Chimera
- User: blewistrumpet-lang
- Email: blewistrumpet@gmail.com

## After Fixing
Your pending commits will push:
- 7 commits ahead of origin/main
- Major Architecture Overhaul commit
- All engine fixes and improvements