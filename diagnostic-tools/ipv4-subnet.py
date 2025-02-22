import ipaddress

def validate_ipv4(ip):
    try:
        # validate the IPv4 address
        ipaddress.IPv4Address(ip)
        return True
    except ValueError:
        return False

def cidr_to_mask(cidr):
    # convert CIDR to subnet mask in 255.255.255.255 format
    mask_bin = '1' * int(cidr) + '0' * (32 - int(cidr))
    mask = [str(int(mask_bin[i:i+8], 2)) for i in range(0, 32, 8)]
    return '.'.join(mask)

def get_network_info(ip, mask):
    try:
        # check if the mask is CIDR format or dotted-decimal format
        if '/' in mask:
            network = ipaddress.IPv4Network(f'{ip}/{mask}', strict=False)
        else:
            network = ipaddress.IPv4Network(f'{ip}/{mask}', strict=False)
        network_address = network.network_address
        broadcast_address = network.broadcast_address
        total_ips = network.num_addresses
        usable_ips = total_ips - 2
        return network_address, broadcast_address, network.prefixlen, usable_ips 
    except ipaddress.NetmaskValueError:
        # handle invalid subnet mask error
        print("Invalid subnet mask. Please enter a valid subnet mask.")
        return None, None, None, None, None, None

def get_input():
    while True:
        ip = input("Please enter an IPv4 address: ")
        if not validate_ipv4(ip):
            print("[Error] The entered IPv4 address is invalid!")
            continue
        mask = input("Please enter the subnet mask (dotted-decimal/CIDR format): ")
        # if the mask is in CIDR notation, convert it to the dotted-decimal format
        if '/' in mask:
            try:
                # validate and convert CIDR to the subnet mask in dotted-decimal format
                cidr = mask.strip('/')
                if not (0 <= int(cidr) <= 32):
                    print("[Error] The CIDR value must be between 0 and 32!")
                    continue
                mask = cidr_to_mask(cidr)
                break
            except ValueError:
                print("[Error] The CIDR notation is invalid!")
        else:
            # ensure mask is a valid 255.255.255.255 format
            mask_parts = mask.split('.')
            if len(mask_parts) != 4 or not all(0 <= int(x) <= 255 for x in mask_parts):
                print("[Error] The subnet mask is invalid!")
                continue
            break
        # validate the mask by checking if it works with ipaddress module
        _, _, _, _, _, _ = get_network_info(ip, mask)
        if _ is not None:
            break
    return ip, mask

def main():
    ip, mask = get_input()
    network_address, broadcast_address, cidr, usable_ips = get_network_info(ip, mask)
    if network_address is None:
        return
    print("\nSUBNET INFORMATION")
    print(f"Input Address: {ip}")
    print(f"Subnet Mask: {mask}/{cidr}")
    print(f"Network Address: {network_address}")
    print(f"Broadcast Address: {broadcast_address}")
    print(f"Total Usable IPv4: {usable_ips}")

if __name__ == "__main__":
    main()