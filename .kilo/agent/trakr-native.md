# Trakr Native Agent Configuration

## Permissions
- **Edit**: Only `native/` directory files
- **Read**: All files for reference
- **Bash**: Build and test commands only

## Constraints
- Visual parity: ≥95.0% pixel similarity required
- Brand parity: No "hardware" or "Polyend" in UI strings

## Workflow
1. Run visual parity tests before and after changes
2. Verify unit tests pass
3. Maintain visual ratchet (no regression)
4. Use FT812 reference coordinates for exact alignment