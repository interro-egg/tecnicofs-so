#!/bin/zsh
# ^ this script doesn't need zsh but the hook does, so might as well make sure it's installed now

# Creates a pre-commit hook that aborts commits which don't adhere to clang-format's standards

cd $(git rev-parse --show-toplevel) # change cwd to working tree root

if [ -f ".git/hooks/pre-commit" ]; then
	echo "A pre-commit hook already exists! Aborting..."
	exit 1
fi

cat <<EOT >> .git/hooks/pre-commit
#!/bin/zsh
if clang-format --dry-run -Werror **/*.c **/*.h &>/dev/null; then
	exit 0
else
	echo ERROR: Formatting check failed! Run "make fmt" to fix
	exit 1
fi
EOT

chmod +x .git/hooks/pre-commit
