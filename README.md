# DropZone — File Sharing Website Setup Guide

A free file-sharing website you can host on GitHub Pages.
- ✅ Anyone can upload & download files (no login)
- 🔐 Only you can delete files (via admin password)
- 💾 Files stored on Cloudinary (free tier: 25GB)
- 📋 File list stored on JSONBin.io (free)

---

## Step 1 — Set Up Cloudinary (file storage)

1. Go to [cloudinary.com](https://cloudinary.com) and create a **free account**
2. On your Dashboard, copy your **Cloud Name** (e.g. `mycloud123`)
3. Go to **Settings → Upload → Upload Presets**
4. Click **Add upload preset**
   - Set **Signing Mode** to `Unsigned`
   - Give it a name like `dropzone_unsigned`
   - Save it
5. Copy the preset name

---

## Step 2 — Set Up JSONBin.io (file registry)

1. Go to [jsonbin.io](https://jsonbin.io) and create a **free account**
2. Click **Create a Bin**
3. Paste this as the initial content:
   ```json
   { "files": [] }
   ```
4. Save it and copy the **Bin ID** from the URL (e.g. `6650a1234abcd`)
5. Go to **API Keys** and copy your **Master Key**

---

## Step 3 — Edit index.html

Open `index.html` and find the CONFIG section near the bottom:

```javascript
const CONFIG = {
  CLOUDINARY_CLOUD_NAME:    "YOUR_CLOUD_NAME",
  CLOUDINARY_UPLOAD_PRESET: "YOUR_UPLOAD_PRESET",
  JSONBIN_BIN_ID:           "YOUR_BIN_ID",
  JSONBIN_API_KEY:          "YOUR_API_KEY",
  ADMIN_PASSWORD:           "YOUR_SECRET_PASSWORD",
};
```

Replace each value:
- `YOUR_CLOUD_NAME` → your Cloudinary cloud name
- `YOUR_UPLOAD_PRESET` → your unsigned preset name
- `YOUR_BIN_ID` → your JSONBin bin ID
- `YOUR_API_KEY` → your JSONBin master key
- `YOUR_SECRET_PASSWORD` → a secret password only you know

---

## Step 4 — Deploy to GitHub Pages

1. Create a new GitHub repository (e.g. `my-dropzone`)
2. Upload `index.html` to the repo
3. Go to **Settings → Pages**
4. Under **Source**, select `main` branch and `/ (root)` folder
5. Click **Save**
6. Your site will be live at `https://yourusername.github.io/my-dropzone/`

---

## How to Use

### For anyone (upload & download):
- Visit the site
- Drag & drop a file or click "Choose File" to upload
- Click "↓ Download" on any file to download it

### For you (admin/delete):
- Click the 🔑 key icon in the bottom-right corner
- Enter your admin password
- Delete buttons appear on every file
- Click ✕ Delete to remove a file from the listing

---

## Notes

- **File size**: Cloudinary free tier supports up to 10MB per upload and 25GB total storage
- **Deletion**: Files are removed from the listing immediately. Full removal from Cloudinary storage requires their dashboard or a backend function
- **Security**: Your admin password is in the HTML file. Don't share the source with others
- **Privacy**: All uploaded files are publicly accessible via their Cloudinary URL

---

## Free Tier Limits

| Service | Free Limit |
|---|---|
| Cloudinary | 25 GB storage, 25 GB bandwidth/month |
| JSONBin.io | 10,000 requests/month |
| GitHub Pages | 1 GB storage, 100 GB bandwidth/month |
