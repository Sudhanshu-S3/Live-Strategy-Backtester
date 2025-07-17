
## 5. Troubleshooting Guide

```markdown
# Troubleshooting Guide

This guide addresses common issues that users might encounter when using the Live Strategy Backtester.

## Installation Issues

### CMake Configuration Errors

**Issue**: CMake fails to configure the build.

**Solution**:
1. Ensure you have the correct CMake version (3.12+)
2. Check that all dependencies are installed
3. Try with a clean build directory:
```bash
rm -rf build
mkdir build && cd build
cmake ..