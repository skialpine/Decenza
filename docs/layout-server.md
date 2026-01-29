# Layout Sharing (Planned)

Share idle screen layouts via decenza.coffee (S3).

## Upload Bundle
- Layout JSON (`Settings.layoutConfiguration`)
- Screenshot of IdlePage (via `grabToImage()`)
- Theme colors (accent, surface, background, etc.)
- Metadata: screen size, author (optional), description

## S3 Structure
```
layouts/
  {uuid}.json     # layout + theme + metadata
  {uuid}.png      # screenshot thumbnail (~400px wide)
  index.json      # manifest listing all layouts
```

## App Flow
- **Upload**: Settings > Layout > "Share" — grabs screenshot, bundles JSON + theme, uploads
- **Browse**: Settings > Layout > "Browse Shared" — fetches index, shows screenshot grid, tap to preview/import
- **Import**: applies layout JSON, optionally applies theme colors

## Notes
- Use Lambda or presigned URLs for upload (prevent abuse)
- No auth needed for download
- Show source screen size/aspect ratio (phone vs tablet layouts differ)
- Thumbnail screenshots for fast gallery loading
- Atomic index.json updates (or use S3 listing API)
- Existing network code (VisualizerUploader, ShotServer) provides HTTP patterns
